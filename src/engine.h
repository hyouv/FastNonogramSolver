// Propagation + full probing + DFS engine (templated on line word count).
//
// State: for every line (rows 0..H-1, then columns) two masks of known black
// and known white cells.  Propagation runs the complete line solver on dirty
// lines until a fixpoint.  Probing tries both values of every unknown cell
// (in place, undone through a trail) and keeps whatever both branches agree
// on; a probe whose touched lines did not change since it last ran is skipped.
#pragma once
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>

#include "bits.h"
#include "linesolve.h"
#include "puzzle.h"

enum Status { S_CONFLICT = 0, S_UNSOLVED = 1, S_SOLVED = 2, S_ABORT = 3 };

struct Stats {
    long nodes = 0, probes = 0, lineSolves = 0, cacheHits = 0, probeSkips = 0;
    int rootKnown = -1;  // cells known after root propagation + probing
};

static inline double now_sec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

template <int NW>
struct Engine {
    using B_t = Bits<NW>;
    using LS = LineSolver<NW>;

    int H = 0, W = 0, L = 0, total = 0, TW = 0;
    std::vector<int> len, coff, ccnt, clues;

    struct State {
        std::vector<B_t> b, w;
        int known = 0;
    };

    // ---- line cache -------------------------------------------------------
    struct CacheEntry {
        B_t kb, kw, ob, ow;
        int line;  // -1 = empty
        int ok;
    };
    std::vector<CacheEntry> cache;
    uint64_t cacheMask = 0;
    bool useCache = true;

    // ---- dirty lines (round-robin scan) -------------------------------------
    std::vector<uint64_t> dirty;
    int ndirty = 0, cursor = 0;
    std::vector<int> fifo;  // FIFO order of dirty lines (ring buffer of size L)
    int fh = 0;
    bool trackTouched = false;
    std::vector<uint64_t> touched;  // TW words

    // ---- undo trail for in-place probing -------------------------------------
    struct TrailEnt {
        int l;
        B_t b, w;
    };
    bool trailing = false;
    uint32_t epoch = 1;
    std::vector<uint32_t> lineEpoch;
    std::vector<TrailEnt> trail, resB;
    std::vector<int> posB;

    // ---- probing bookkeeping ---------------------------------------------
    uint32_t gstamp = 1;
    std::vector<uint32_t> lineStamp;
    struct ProbeInfo {
        uint32_t stamp;  // 0 = invalid
        int gainB, gainW;
    };
    std::vector<ProbeInfo> pinfo;
    std::vector<uint64_t> ptouched;  // cells * TW

    Stats st;
    double deadline = 1e300;
    long nodeLimit = -1, probeLimit = -1;
    int maxSolutions = 1;
    std::vector<std::vector<uint8_t>> solutions;
    bool aborted = false;

    // restarts: a run is abandoned after restartBudget nodes (-1 = never)
    long restartBudget = -1, runNodes = 0;
    bool restartHit = false;
    double noise = 0.0;
    uint64_t rngState = 0x2545F4914F6CDD1DULL;
    int branchHeur = 0;  // 0: LalaFrogKK score, 1: product, 2: min then max, 3: sum
    int valueOrder = 0;  // 0: white first, 1: black first, 2: larger gain first, 3: smaller gain first

    // per-depth save areas for DFS
    std::vector<std::vector<ProbeInfo>> saveInfo;
    std::vector<std::vector<uint64_t>> saveTouched;
    std::vector<std::vector<uint32_t>> saveStamp;
    std::deque<State> childState;  // deque: references stay valid while growing

    void init(const Puzzle& p, int cacheBits = 16) {
        H = p.H;
        W = p.W;
        L = H + W;
        total = H * W;
        TW = (L + 63) / 64;
        len.resize(L);
        coff.resize(L);
        ccnt.resize(L);
        clues.clear();
        for (int l = 0; l < L; l++) {
            const auto& c = l < H ? p.rows[l] : p.cols[l - H];
            len[l] = l < H ? W : H;
            coff[l] = (int)clues.size();
            ccnt[l] = (int)c.size();
            for (int x : c) clues.push_back(x);
        }
        useCache = cacheBits > 0;
        if (cacheBits <= 0) cacheBits = 1;
        cache.assign(size_t(1) << cacheBits, CacheEntry{});
        for (auto& e : cache) e.line = -1;
        cacheMask = (uint64_t(1) << cacheBits) - 1;
        dirty.assign(TW, 0);
        ndirty = cursor = 0;
        fifo.assign(L, 0);
        fh = 0;
        touched.assign(TW, 0);
        lineEpoch.assign(L, 0);
        trail.clear();
        trail.reserve(L);
        resB.clear();
        resB.reserve(L);
        posB.assign(L, -1);
        lineStamp.assign(L, 0);
        pinfo.assign(total, ProbeInfo{0, 0, 0});
        ptouched.assign((size_t)total * TW, 0);
    }

    void newState(State& s) const {
        s.b.assign(L, B_t::zero());
        s.w.assign(L, B_t::zero());
        s.known = 0;
    }

    inline int cellState(const State& s, int r, int c) const {  // -1 unknown, 0 white, 1 black
        if (s.b[r].test(c)) return 1;
        if (s.w[r].test(c)) return 0;
        return -1;
    }

    inline void push(int l) {
        uint64_t m = 1ULL << (l & 63);
        if (dirty[l >> 6] & m) return;
        dirty[l >> 6] |= m;
        int pos = fh + ndirty;
        if (pos >= L) pos -= L;
        fifo[pos] = l;
        ndirty++;
    }
    inline int pop() {
        int l = fifo[fh];
        if (++fh == L) fh = 0;
        dirty[l >> 6] &= ~(1ULL << (l & 63));
        ndirty--;
        return l;
    }
    void clearQueue() {
        for (int i = 0; i < TW; i++) dirty[i] = 0;
        ndirty = 0;
        fh = 0;
    }
    void pushAll() {
        for (int l = 0; l < L; l++) push(l);
    }

    inline void save(State& s, int l) {
        if (trailing && lineEpoch[l] != epoch) {
            lineEpoch[l] = epoch;
            trail.push_back(TrailEnt{l, s.b[l], s.w[l]});
        }
    }
    inline void undo(State& s) {
        for (const TrailEnt& t : trail) {
            s.b[t.l] = t.b;
            s.w[t.l] = t.w;
        }
    }

    inline uint64_t hashKey(int l, const B_t& b, const B_t& w) const {
        uint64_t h = (uint64_t)(l + 1) * 0x9E3779B97F4A7C15ULL;
        for (int i = 0; i < NW; i++) h = (h ^ b.w[i]) * 0xBF58476D1CE4E5B9ULL ^ (w.w[i] * 0x94D049BB133111EBULL);
        return h ^ (h >> 29);
    }

    // Solve line l under (b, w).  Returns false on conflict.
    inline bool solveLine(int l, const B_t& b, const B_t& w, B_t& ob, B_t& ow) {
        if (!useCache) {
            st.lineSolves++;
            return LS::solve(len[l], &clues[coff[l]], ccnt[l], b, w, ob, ow);
        }
        CacheEntry& e = cache[hashKey(l, b, w) & cacheMask];
        if (e.line == l && e.kb == b && e.kw == w) {
            st.cacheHits++;
            ob = e.ob;
            ow = e.ow;
            return e.ok;
        }
        st.lineSolves++;
        bool ok = LS::solve(len[l], &clues[coff[l]], ccnt[l], b, w, ob, ow);
        e.line = l;
        e.kb = b;
        e.kw = w;
        e.ob = ob;
        e.ow = ow;
        e.ok = ok;
        return ok;
    }

    // Run dirty lines to a fixpoint.  Returns false on conflict.
    bool propagate(State& s) {
        while (ndirty) {
            int l = pop();
            if (trackTouched) touched[l >> 6] |= 1ULL << (l & 63);
            B_t ob, ow;
            if (!solveLine(l, s.b[l], s.w[l], ob, ow)) {
                clearQueue();
                return false;
            }
            B_t nb = ob.andnot(s.b[l]), nw = ow.andnot(s.w[l]);
            if (nb.none() && nw.none()) continue;
            save(s, l);
            s.b[l] = ob | s.b[l];
            s.w[l] = ow | s.w[l];
            s.known += nb.popcount() + nw.popcount();
            if (l < H) {
                int r = l;
                nb.foreach_bit([&](int c) {
                    save(s, H + c);
                    s.b[H + c].set(r);
                    push(H + c);
                });
                nw.foreach_bit([&](int c) {
                    save(s, H + c);
                    s.w[H + c].set(r);
                    push(H + c);
                });
            } else {
                int c = l - H;
                nb.foreach_bit([&](int r) {
                    save(s, r);
                    s.b[r].set(c);
                    push(r);
                });
                nw.foreach_bit([&](int r) {
                    save(s, r);
                    s.w[r].set(c);
                    push(r);
                });
            }
        }
        return true;
    }

    inline void assign(State& s, int r, int c, int v) {
        save(s, r);
        save(s, H + c);
        if (v) {
            s.b[r].set(c);
            s.b[H + c].set(r);
        } else {
            s.w[r].set(c);
            s.w[H + c].set(r);
        }
        s.known++;
        push(r);
        push(H + c);
    }

    // Mark lines that differ between old and cur with a fresh stamp.
    void stampChanges(const State& old, const State& cur) {
        gstamp++;
        for (int l = 0; l < L; l++)
            if (old.b[l] != cur.b[l] || old.w[l] != cur.w[l]) lineStamp[l] = gstamp;
    }

    bool probeStillValid(int cell) const {
        const ProbeInfo& pi = pinfo[cell];
        if (pi.stamp == 0) return false;
        const uint64_t* t = &ptouched[(size_t)cell * TW];
        for (int i = 0; i < TW; i++) {
            uint64_t x = t[i];
            while (x) {
                int l = 64 * i + __builtin_ctzll(x);
                if (lineStamp[l] > pi.stamp) return false;
                x &= x - 1;
            }
        }
        return true;
    }

    void recordSolution(const State& s) {
        std::vector<uint8_t> g(total);
        for (int r = 0; r < H; r++)
            for (int c = 0; c < W; c++) g[r * W + c] = s.b[r].test(c);
        for (auto& x : solutions)
            if (x == g) return;
        solutions.push_back(std::move(g));
    }

    bool timeUp() {
        if ((st.probes & 255) == 0 && now_sec() > deadline) aborted = true;
        return aborted;
    }

    // Probe every unknown cell until no new information.  On SOLVED in
    // single-solution mode, s holds the solution.
    Status probeFixpoint(State& s) {
        for (;;) {
            bool changed = false;
            for (int r = 0; r < H; r++) {
                for (int c = 0; c < W; c++) {
                    if (s.b[r].test(c) || s.w[r].test(c)) continue;
                    int cell = r * W + c;
                    if (probeStillValid(cell)) {
                        st.probeSkips++;
                        continue;
                    }
                    if (timeUp()) return S_ABORT;
                    st.probes++;
                    const int known0 = s.known;
                    for (int i = 0; i < TW; i++) touched[i] = 0;
                    trackTouched = true;
                    trailing = true;
                    // --- black ---
                    epoch++;
                    trail.clear();
                    assign(s, r, c, 1);
                    bool okB = propagate(s);
                    int knownB = s.known;
                    resB.clear();
                    if (okB) {
                        for (const TrailEnt& t : trail) {
                            posB[t.l] = (int)resB.size();
                            resB.push_back(TrailEnt{t.l, s.b[t.l], s.w[t.l]});
                        }
                        if (knownB == total) {
                            recordSolution(s);
                            if ((int)solutions.size() >= maxSolutions) {
                                trailing = trackTouched = false;
                                for (auto& t : resB) posB[t.l] = -1;
                                return S_SOLVED;
                            }
                        }
                    }
                    undo(s);
                    s.known = known0;
                    // --- white ---
                    epoch++;
                    trail.clear();
                    assign(s, r, c, 0);
                    bool okW = propagate(s);
                    int knownW = s.known;
                    trailing = trackTouched = false;
                    uint64_t* pt = &ptouched[(size_t)cell * TW];
                    for (int i = 0; i < TW; i++) pt[i] = touched[i];
                    if (okW && knownW == total) {
                        recordSolution(s);
                        if ((int)solutions.size() >= maxSolutions) {
                            for (auto& t : resB) posB[t.l] = -1;
                            return S_SOLVED;
                        }
                    }
                    if (!okB && !okW) {
                        undo(s);
                        s.known = known0;
                        return S_CONFLICT;
                    }
                    if (!okB || !okW) {
                        gstamp++;
                        if (okW) {  // keep the white result in place
                            for (const TrailEnt& t : trail) lineStamp[t.l] = gstamp;
                        } else {  // restore and re-apply the black result
                            undo(s);
                            for (const TrailEnt& t : resB) {
                                s.b[t.l] = t.b;
                                s.w[t.l] = t.w;
                                lineStamp[t.l] = gstamp;
                            }
                            s.known = knownB;
                        }
                        for (auto& t : resB) posB[t.l] = -1;
                        pinfo[cell].stamp = 0;
                        changed = true;
                        if (s.known == total) return S_SOLVED;  // already recorded
                        continue;
                    }
                    pinfo[cell].stamp = gstamp;
                    pinfo[cell].gainB = knownB - known0;
                    pinfo[cell].gainW = knownW - known0;
                    // Intersection of the two fixpoints (itself a fixpoint).
                    // Only lines modified by both probes can gain information;
                    // every other line returns to its value before the probes.
                    bool any = false;
                    int gain = 0;
                    for (const TrailEnt& t : trail) {
                        int pb = posB[t.l];
                        if (pb < 0) {
                            s.b[t.l] = t.b;
                            s.w[t.l] = t.w;
                            continue;
                        }
                        B_t nb = resB[pb].b & s.b[t.l];
                        B_t nw = resB[pb].w & s.w[t.l];
                        s.b[t.l] = nb;
                        s.w[t.l] = nw;
                        if (nb != t.b || nw != t.w) {
                            if (!any) {
                                gstamp++;
                                any = true;
                            }
                            lineStamp[t.l] = gstamp;
                            if (t.l < H) gain += nb.andnot(t.b).popcount() + nw.andnot(t.w).popcount();
                        }
                    }
                    s.known = known0 + gain;
                    if (any) changed = true;
                    for (auto& t : resB) posB[t.l] = -1;
                }
            }
            if (!changed) break;
        }
        return s.known == total ? S_SOLVED : S_UNSOLVED;
    }

    // Binary clauses (X=v -> Y=u) for every single-cell implication found by
    // propagation from state s.  Cell (r,c) is SAT variable r*W+c+1.
    void implications(State& s, std::vector<int>& out, size_t maxLits) {
        const int known0 = s.known;
        for (int r = 0; r < H; r++)
            for (int c = 0; c < W; c++) {
                if (s.b[r].test(c) || s.w[r].test(c)) continue;
                int x = r * W + c + 1;
                for (int v = 0; v < 2; v++) {
                    trailing = true;
                    epoch++;
                    trail.clear();
                    assign(s, r, c, v);
                    bool ok = propagate(s);
                    trailing = false;
                    int nx = v ? -x : x;  // clause literal "not (X = v)"
                    if (!ok) {
                        out.push_back(nx);
                        out.push_back(0);
                    } else {
                        for (const TrailEnt& t : trail) {
                            if (t.l >= H) continue;  // rows cover every cell
                            int rr = t.l;
                            B_t nb = s.b[rr].andnot(t.b), nw = s.w[rr].andnot(t.w);
                            nb.foreach_bit([&](int cc) {
                                if (rr == r && cc == c) return;
                                out.push_back(nx);
                                out.push_back(rr * W + cc + 1);
                                out.push_back(0);
                            });
                            nw.foreach_bit([&](int cc) {
                                if (rr == r && cc == c) return;
                                out.push_back(nx);
                                out.push_back(-(rr * W + cc + 1));
                                out.push_back(0);
                            });
                        }
                    }
                    undo(s);
                    s.known = known0;
                    if (out.size() > maxLits) return;
                }
            }
    }

    inline double urand() {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 7;
        rngState ^= rngState << 17;
        return (rngState >> 11) * (1.0 / 9007199254740992.0);
    }

    int chooseCell(const State& s) {
        int best = -1;
        double bestScore = -1;
        for (int r = 0; r < H; r++)
            for (int c = 0; c < W; c++) {
                if (s.b[r].test(c) || s.w[r].test(c)) continue;
                const ProbeInfo& pi = pinfo[r * W + c];
                double a = pi.gainB, b = pi.gainW, score;
                switch (branchHeur) {
                    case 1: score = (a + 1) * (b + 1); break;
                    case 2: score = std::min(a, b) * 4096 + std::max(a, b); break;
                    case 3: score = a + b; break;
                    default: score = std::min(a, b) + 1.85 * std::log(1.0 + std::fabs(a - b));
                }
                if (noise > 0) score *= 1.0 + noise * urand();
                if (score > bestScore) {
                    bestScore = score;
                    best = r * W + c;
                }
            }
        return best;
    }
    int firstValue(int cell) const {
        const ProbeInfo& pi = pinfo[cell];
        switch (valueOrder) {
            case 1: return 1;
            case 2: return pi.gainB >= pi.gainW ? 1 : 0;
            case 3: return pi.gainB < pi.gainW ? 1 : 0;
            default: return 0;
        }
    }

    // Deepest probed DFS node so far (most known cells), a phase hint for CDCL.
    bool trackBest = false;
    State best;

    // Depth-first search with probing at every node.
    Status dfs(State& s, int depth) {
        st.nodes++;
        if ((nodeLimit >= 0 && st.nodes > nodeLimit) || (probeLimit >= 0 && st.probes > probeLimit)) {
            aborted = true;
            return S_ABORT;
        }
        if (restartBudget >= 0 && ++runNodes > restartBudget) {
            restartHit = true;
            return S_ABORT;
        }
        Status ps = probeFixpoint(s);
        if (ps == S_ABORT) return S_ABORT;
        if (ps == S_CONFLICT) return S_CONFLICT;
        if (ps == S_SOLVED) {
            recordSolution(s);
            return (int)solutions.size() >= maxSolutions ? S_SOLVED : S_CONFLICT;
        }
        if (trackBest && s.known > best.known) best = s;
        int cell = chooseCell(s);
        int r = cell / W, c = cell % W;
        if ((int)saveInfo.size() <= depth) {
            saveInfo.resize(depth + 1);
            saveTouched.resize(depth + 1);
            saveStamp.resize(depth + 1);
            childState.resize(depth + 1);
        }
        saveInfo[depth] = pinfo;
        saveTouched[depth] = ptouched;
        saveStamp[depth] = lineStamp;
        int v0 = firstValue(cell);
        for (int k = 0; k < 2; k++) {
            int v = (k == 0) ? v0 : 1 - v0;
            State& child = childState[depth];
            child = s;
            assign(child, r, c, v);
            if (propagate(child)) {
                stampChanges(s, child);
                Status cs;
                if (child.known == total) {
                    recordSolution(child);
                    cs = (int)solutions.size() >= maxSolutions ? S_SOLVED : S_CONFLICT;
                } else {
                    cs = dfs(child, depth + 1);
                }
                if (cs == S_SOLVED) s = child;
                if (cs == S_SOLVED || cs == S_ABORT) return cs;
            }
            if (k == 0) {
                pinfo = saveInfo[depth];
                ptouched = saveTouched[depth];
                lineStamp = saveStamp[depth];
            }
        }
        return S_CONFLICT;
    }

    // Root: propagate all lines, then probe + search.
    Status solveRoot(State& s, bool search = true) {
        newState(s);
        pushAll();
        if (!propagate(s)) return S_CONFLICT;
        if (s.known == total) {
            recordSolution(s);
            return S_SOLVED;
        }
        if (!search) return S_UNSOLVED;
        Status r = dfs(s, 0);
        if (r == S_ABORT) return S_ABORT;
        return solutions.empty() ? S_CONFLICT : S_SOLVED;
    }
};

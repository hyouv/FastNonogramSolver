// CNF encoding of a (partially solved) nonogram and CDCL search via CaDiCaL.
//
// Each line is encoded as its automaton  0* 1^c1 0+ 1^c2 ... 1^ck 0*  unrolled
// over the cells.  Block-interior automaton states are deterministic in both
// directions, so they collapse into one "block j starts at p" variable; gap
// states keep one variable per (gap, layer).  With incoming/outgoing support
// clauses and value-support clauses, unit propagation on this encoding is as
// strong as a complete line solver (GAC), while learned clauses can talk about
// block positions as well as cells.
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "puzzle.h"

struct CNF {
    int nvars = 0;
    std::vector<int> lits;  // clauses separated by 0
    long nclauses = 0;
    inline int newVar() { return ++nvars; }
    inline void add(std::initializer_list<int> c) {
        for (int x : c) lits.push_back(x);
        lits.push_back(0);
        nclauses++;
    }
    inline void add(const std::vector<int>& c) {
        for (int x : c) lits.push_back(x);
        lits.push_back(0);
        nclauses++;
    }
};

// Encoding variants (for experiments):
//   0: automaton (gap states + block starts), GAC by unit propagation
//   1: variant 0 plus order variables "block j starts at or before p"
//   2: order variables + cover variables only (block-position encoding)
inline int g_encoding = 2;
inline int g_seed = 0;
inline std::string g_kissatConfig;  // e.g. "sat", "unsat"; empty = default
inline int g_cadicalConflicts = 20000;  // budget for solution-guided CaDiCaL before Kissat

static bool encodeLineOrder(CNF& F, int n, const int* c, int k, const int* known, const int* xv);

// known: -1 unknown, 0 white, 1 black for each cell of the line.
// xv: variable of each cell.  Returns false if the line is infeasible.
static bool encodeLine(CNF& F, int n, const int* c, int k, const int* known, const int* xv) {
    if (g_encoding == 2) return encodeLineOrder(F, n, c, k, known, xv);
    auto cellOK = [&](int i, int v) { return known[i] < 0 || known[i] == v; };
    // prefix count of white-known cells for O(1) "block fits" test
    std::vector<int> wpre(n + 1, 0);
    for (int i = 0; i < n; i++) wpre[i + 1] = wpre[i] + (known[i] == 0);
    auto blockFits = [&](int j, int p) {  // block j on [p, p+c) and separator
        int e = p + c[j];
        if (p < 0 || e > n) return false;
        if (wpre[e] - wpre[p]) return false;
        if (e < n && !cellOK(e, 0)) return false;
        if (e == n && j != k - 1) return false;
        return true;
    };
    // gap layers: gap j at layer i (0..n).  Forward reachability.
    std::vector<std::vector<char>> gf(k + 1, std::vector<char>(n + 2, 0)), gb = gf;
    std::vector<std::vector<char>> sf(k, std::vector<char>(n + 1, 0)), sb = sf;
    bool endBlockF = false;  // block k-1 ending exactly at n reachable
    gf[0][0] = 1;
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= k; j++) {
            if (!gf[j][i]) continue;
            if (i < n && cellOK(i, 0)) gf[j][i + 1] = 1;
            if (j < k && i < n && cellOK(i, 1) && blockFits(j, i)) {
                sf[j][i] = 1;
                int e = i + c[j];
                if (e == n) endBlockF = true;
                else gf[j + 1][e + 1] = 1;
            }
        }
    }
    // backward reachability from accepting states
    bool acceptGap = gf[k][n];
    if (!acceptGap && !endBlockF) return false;
    gb[k][n] = acceptGap;
    for (int i = n; i >= 0; i--) {
        for (int j = k; j >= 0; j--) {
            if (!gf[j][i]) continue;
            bool live = (i == n) ? (j == k && acceptGap) : false;
            if (i < n && cellOK(i, 0) && gb[j][i + 1]) live = true;
            if (j < k && sf[j][i]) {
                int e = i + c[j];
                bool ok = (e == n) ? true : gb[j + 1][e + 1];
                if (ok) {
                    sb[j][i] = 1;
                    live = true;
                }
            }
            gb[j][i] = live;
        }
    }
    if (!gb[0][0]) return false;
    // variables
    std::vector<std::vector<int>> G(k + 1, std::vector<int>(n + 1, 0)), S(k, std::vector<int>(n + 1, 0));
    const int TRUEV = 0;  // marker: g[0][0] is constant true
    for (int j = 0; j <= k; j++)
        for (int i = 0; i <= n; i++)
            if (gb[j][i] && !(j == 0 && i == 0)) G[j][i] = F.newVar();
    for (int j = 0; j < k; j++)
        for (int p = 0; p < n; p++)
            if (sb[j][p]) S[j][p] = F.newVar();
    (void)TRUEV;
    // literal for gap state; returns 0 if constant true, INT_MIN if dead
    auto gl = [&](int j, int i) -> int {
        if (j == 0 && i == 0) return 0;
        if (j < 0 || j > k || i < 0 || i > n) return INT32_MIN;
        return gb[j][i] ? G[j][i] : INT32_MIN;
    };
    auto add = [&](std::vector<int> cl) {
        // drop constant-true literal handling: a clause containing a "true" lit is satisfied
        std::vector<int> out;
        for (int x : cl) {
            if (x == INT32_MIN) continue;  // dead (false) literal: drop
            if (x == INT32_MAX) return;    // true literal: clause satisfied
            out.push_back(x);
        }
        F.add(out);
    };
    auto gpos = [&](int j, int i) -> int {  // positive literal of gap state
        int v = gl(j, i);
        if (v == 0) return INT32_MAX;
        return v;
    };
    auto gneg = [&](int j, int i) -> int {
        int v = gl(j, i);
        if (v == 0) return INT32_MIN;      // not true = false
        if (v == INT32_MIN) return INT32_MAX;  // not dead = true
        return -v;
    };
    auto spos = [&](int j, int p) -> int {
        if (j < 0 || j >= k || p < 0 || p >= n || !sb[j][p]) return INT32_MIN;
        return S[j][p];
    };
    // gap states: outgoing and incoming support
    for (int j = 0; j <= k; j++) {
        for (int i = 0; i <= n; i++) {
            if (!gb[j][i]) continue;
            int g = gneg(j, i);  // "not g"
            if (i < n) {
                int x = xv[i];
                // g & ~x -> g[j][i+1] ;  g & x -> s[j][i]
                add({g, x, gpos(j, i + 1)});
                add({g, -x, spos(j, i)});
                add({g, gpos(j, i + 1), spos(j, i)});
            } else if (j != k) {
                add({g});  // non-accepting at the end
            }
            if (i > 0) {
                // incoming: previous cell white, came from same gap or from separator after block j-1
                add({g, -xv[i - 1]});
                std::vector<int> cl = {g, gpos(j, i - 1)};
                if (j >= 1) cl.push_back(spos(j - 1, i - 1 - c[j - 1]));
                add(cl);
            }
        }
    }
    // block starts
    for (int j = 0; j < k; j++) {
        for (int p = 0; p < n; p++) {
            if (!sb[j][p]) continue;
            int s = S[j][p];
            for (int t = 0; t < c[j]; t++) add({-s, xv[p + t]});
            int e = p + c[j];
            if (e < n) {
                add({-s, -xv[e]});
                add({-s, gpos(j + 1, e + 1)});
            }
            add({-s, gpos(j, p)});  // incoming: gap j at layer p
        }
    }
    // value support: cell black -> some block covers it; cell white -> some gap state at layer i+1
    for (int i = 0; i < n; i++) {
        std::vector<int> cb = {-xv[i]};
        for (int j = 0; j < k; j++)
            for (int p = std::max(0, i - c[j] + 1); p <= i; p++)
                if (sb[j][p]) cb.push_back(S[j][p]);
        add(cb);
        std::vector<int> cw = {xv[i]};
        for (int j = 0; j <= k; j++) cw.push_back(gpos(j, i + 1));
        add(cw);
    }
    // acceptance
    {
        std::vector<int> cl = {gpos(k, n)};
        if (k > 0) cl.push_back(spos(k - 1, n - c[k - 1]));
        add(cl);
    }
    if (g_encoding == 1) {
        // order variables y[j][p] <-> start_j <= p, for p in [lo_j, hi_j)
        std::vector<int> lo(k), hi(k);
        std::vector<std::vector<int>> Y(k);
        for (int j = 0; j < k; j++) {
            lo[j] = n;
            hi[j] = -1;
            for (int p = 0; p < n; p++)
                if (sb[j][p]) {
                    lo[j] = std::min(lo[j], p);
                    hi[j] = std::max(hi[j], p);
                }
            Y[j].assign(n + 1, 0);
            for (int p = lo[j]; p < hi[j]; p++) Y[j][p] = F.newVar();
        }
        // literal for (start_j <= p): INT32_MAX true, INT32_MIN false
        auto ylit = [&](int j, int p) -> int {
            if (p < lo[j]) return INT32_MIN;
            if (p >= hi[j]) return INT32_MAX;
            return Y[j][p];
        };
        auto neg = [](int x) -> int {
            if (x == INT32_MAX) return INT32_MIN;
            if (x == INT32_MIN) return INT32_MAX;
            return -x;
        };
        for (int j = 0; j < k; j++) {
            for (int p = lo[j]; p < hi[j]; p++) add({neg(ylit(j, p)), ylit(j, p + 1)});
            for (int p = lo[j]; p <= hi[j]; p++) {
                int sp = sb[j][p] ? S[j][p] : INT32_MIN;
                if (sp != INT32_MIN) {
                    add({-sp, ylit(j, p)});
                    add({-sp, neg(ylit(j, p - 1))});
                }
                add({neg(ylit(j, p)), ylit(j, p - 1), sp});
            }
            if (j + 1 < k)
                for (int p = lo[j + 1]; p <= hi[j + 1]; p++)
                    add({neg(ylit(j + 1, p)), ylit(j, p - c[j] - 1)});
        }
    }
    return true;
}

// Block-position encoding: order variables y[j][p] (start_j <= p) and cover
// variables v[j][i] (block j covers cell i);  x_i <-> OR_j v[j][i].
static bool encodeLineOrder(CNF& F, int n, const int* c, int k, const int* known, const int* xv) {
    auto add = [&](std::vector<int> cl) {
        std::vector<int> out;
        for (int x : cl) {
            if (x == INT32_MIN) continue;
            if (x == INT32_MAX) return;
            out.push_back(x);
        }
        F.add(out);
    };
    if (k == 0) {
        for (int i = 0; i < n; i++) add({-xv[i]});
        return true;
    }
    // earliest / latest starts from the clue alone, then tightened by a pass
    std::vector<int> lo(k), hi(k);
    int pos = 0;
    for (int j = 0; j < k; j++) {
        lo[j] = pos;
        pos += c[j] + 1;
    }
    pos = n;
    for (int j = k - 1; j >= 0; j--) {
        hi[j] = pos - c[j];
        pos = hi[j] - 1;
    }
    for (int j = 0; j < k; j++)
        if (lo[j] > hi[j]) return false;
    std::vector<std::vector<int>> Y(k, std::vector<int>(n + 1, 0));
    for (int j = 0; j < k; j++)
        for (int p = lo[j]; p < hi[j]; p++) Y[j][p] = F.newVar();
    auto ylit = [&](int j, int p) -> int {
        if (p < lo[j]) return INT32_MIN;
        if (p >= hi[j]) return INT32_MAX;
        return Y[j][p];
    };
    auto neg = [](int x) -> int {
        if (x == INT32_MAX) return INT32_MIN;
        if (x == INT32_MIN) return INT32_MAX;
        return -x;
    };
    for (int j = 0; j < k; j++)
        for (int p = lo[j]; p < hi[j]; p++) add({neg(ylit(j, p)), ylit(j, p + 1)});
    for (int j = 0; j + 1 < k; j++)
        for (int p = lo[j + 1]; p <= hi[j + 1]; p++) add({neg(ylit(j + 1, p)), ylit(j, p - c[j] - 1)});
    // cover variables
    std::vector<std::vector<int>> cov(n);
    for (int j = 0; j < k; j++) {
        for (int i = lo[j]; i < hi[j] + c[j]; i++) {
            // v <-> (start <= i) & !(start <= i - c)
            int a = ylit(j, i), b = neg(ylit(j, i - c[j]));
            int v = F.newVar();
            add({-v, a});
            add({-v, b});
            add({v, neg(a), neg(b)});
            add({-v, xv[i]});
            cov[i].push_back(v);
        }
    }
    for (int i = 0; i < n; i++) {
        std::vector<int> cl = {-xv[i]};
        for (int v : cov[i]) cl.push_back(v);
        add(cl);
    }
    (void)known;
    return true;
}

// Encode the whole puzzle.  Cell (r,c) is variable r*W+c+1.  known has H*W
// entries (-1/0/1); known cells become unit clauses.
static bool encodePuzzle(const Puzzle& p, const std::vector<int8_t>& known, CNF& F) {
    F = CNF();
    F.nvars = p.H * p.W;
    std::vector<int> kn, xv;
    for (int r = 0; r < p.H; r++) {
        kn.assign(p.W, -1);
        xv.assign(p.W, 0);
        for (int c = 0; c < p.W; c++) {
            kn[c] = known[r * p.W + c];
            xv[c] = r * p.W + c + 1;
        }
        if (!encodeLine(F, p.W, p.rows[r].data(), (int)p.rows[r].size(), kn.data(), xv.data())) return false;
    }
    for (int c = 0; c < p.W; c++) {
        kn.assign(p.H, -1);
        xv.assign(p.H, 0);
        for (int r = 0; r < p.H; r++) {
            kn[r] = known[r * p.W + c];
            xv[r] = r * p.W + c + 1;
        }
        if (!encodeLine(F, p.H, p.cols[c].data(), (int)p.cols[c].size(), kn.data(), xv.data())) return false;
    }
    for (int i = 0; i < p.H * p.W; i++)
        if (known[i] >= 0) F.add({known[i] ? i + 1 : -(i + 1)});
    return true;
}

enum SatResult { SAT_UNSAT = 0, SAT_DONE = 1, SAT_TIMEOUT = 2 };

// Resumable CaDiCaL session (for interleaving with DFS).
struct IncSat;
IncSat* incsatCreate(const Puzzle& p, const std::vector<int8_t>& known, double deadline);  // null: no solution
int incsatSolve(IncSat* S, long conflicts);  // 10 SAT, 20 UNSAT, 0 budget/time exhausted
void incsatModel(IncSat* S, std::vector<uint8_t>& g);
void incsatDestroy(IncSat* S);

// Find solutions until `want` distinct ones are known (including `found`).
SatResult satSearch(const Puzzle& p, const std::vector<int8_t>& known,
                    std::vector<std::vector<uint8_t>>& found, int want, double deadline,
                    bool verbose, const std::string& backend, const std::vector<int>* extra = nullptr);

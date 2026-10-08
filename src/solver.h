// Top-level strategy: line solving -> probing -> bounded DFS -> CDCL.
#pragma once
#include <string>
#include <vector>

#include "engine.h"
#include "puzzle.h"
#include "sat.h"

struct Options {
    int maxSolutions = 2;
    bool maxSolutionsSet = false;
    double timeout = 1e9;
    std::string sat = "auto";  // auto: hybrid for a single solution, kissat otherwise
    long dfsNodes = 100000;
    double dfsWork = 40e6;  // DFS budget in probes x lines before switching to CDCL
    bool verbose = false;
    std::string dimacs;
    int branchHeur = 0, valueOrder = 0;
    long nodeLimit = -1;  // hard DFS node limit when sat == none
    long restartBase = 300;  // >0: Luby restarts with this many nodes per unit
    double noise = 0.5;
    int cacheBits = -1;  // -1 auto, 0 disables the line cache
    int encoding = 2;
    int seed = 0;
    int cadicalConflicts = 20000;
    std::string kissatConfig;
    int satPhase = 2;  // phase hints: 0 none, 1 per-line counts, 2 belief propagation
    bool satStarts = true, satDfsPhase = false;
    int bpIters = 20;
    std::vector<std::pair<std::string, int>> kissatOpts;
    bool interleave = false;
    bool probeClauses = true;
    double probeClauseLits = 2e7;  // single-solution mode: alternate DFS and CaDiCaL
    double dfsWork1 = 8e6;
    long satConflicts1 = 2000;  // if set: write CNF after root probing and stop
};

enum ResultStatus { R_NONE, R_UNIQUE, R_MULTIPLE, R_FOUND, R_TIMEOUT, R_ERROR };

struct Result {
    ResultStatus status = R_ERROR;
    std::vector<std::vector<uint8_t>> solutions;
    Stats stats;
    bool usedSat = false;
    const char* statusName() const {
        switch (status) {
            case R_NONE: return "NONE";
            case R_UNIQUE: return "UNIQUE";
            case R_MULTIPLE: return "MULTIPLE";
            case R_FOUND: return "FOUND";
            case R_TIMEOUT: return "TIMEOUT";
            default: return "ERROR";
        }
    }
};

static bool verifySolution(const Puzzle& p, const std::vector<uint8_t>& g) {
    auto runs = [](const std::vector<int>& v) {
        std::vector<int> r;
        int k = 0;
        for (int x : v) {
            if (x) k++;
            else if (k) {
                r.push_back(k);
                k = 0;
            }
        }
        if (k) r.push_back(k);
        return r;
    };
    std::vector<int> line;
    for (int r = 0; r < p.H; r++) {
        line.assign(g.begin() + r * p.W, g.begin() + (r + 1) * p.W);
        if (runs(line) != p.rows[r]) return false;
    }
    for (int c = 0; c < p.W; c++) {
        line.clear();
        for (int r = 0; r < p.H; r++) line.push_back(g[r * p.W + c]);
        if (runs(line) != p.cols[c]) return false;
    }
    return true;
}

template <int NW>
Result solveT(const Puzzle& p, const Options& opt) {
    Result res;
    g_encoding = opt.encoding;
    g_seed = opt.seed;
    g_cadicalConflicts = opt.cadicalConflicts;
    g_kissatConfig = opt.kissatConfig;
    g_satPhase = opt.satPhase;
    g_satStarts = opt.satStarts;
    g_kissatOpts = opt.kissatOpts;
    g_hintKnown.clear();
    g_bpIters = opt.bpIters;
    double deadline = now_sec() + opt.timeout;
    int cacheBits = p.H * p.W <= 1024 ? 18 : 20;
    if (NW >= 3) cacheBits = 19;
    if (opt.cacheBits >= 0) cacheBits = opt.cacheBits;
    Engine<NW> E;
    E.init(p, cacheBits);
    E.maxSolutions = opt.maxSolutions;
    E.branchHeur = opt.branchHeur;
    E.valueOrder = opt.valueOrder;
    E.deadline = deadline;
    typename Engine<NW>::State s;
    Status st = E.solveRoot(s, false);
    auto finish = [&](bool complete) {
        res.solutions = E.solutions;
        res.stats = E.st;
        int n = (int)res.solutions.size();
        if (n == 0) res.status = complete ? R_NONE : R_TIMEOUT;
        else if (n >= 2) res.status = R_MULTIPLE;
        else if (opt.maxSolutions <= 1) res.status = R_FOUND;
        else res.status = complete ? R_UNIQUE : R_TIMEOUT;
        for (auto& g : res.solutions)
            if (!verifySolution(p, g)) res.status = R_ERROR;
        return res;
    };
    if (st == S_CONFLICT) return finish(true);
    if (st == S_SOLVED) return finish(true);
    // root probing
    Status ps = E.probeFixpoint(s);
    if (ps == S_ABORT) return finish(false);
    if (ps == S_CONFLICT) return finish(true);
    if (ps == S_SOLVED) {
        E.recordSolution(s);
        if ((int)E.solutions.size() >= opt.maxSolutions) return finish(true);
        // remaining solutions (if any) differ from s, but s is fully determined
        // by root deductions, so it is the only one.
        return finish(true);
    }
    typename Engine<NW>::State root = s;
    E.st.rootKnown = root.known;
    if (!opt.dimacs.empty()) {
        std::vector<int8_t> known(p.H * p.W, -1);
        for (int r = 0; r < p.H; r++)
            for (int c = 0; c < p.W; c++) known[r * p.W + c] = (int8_t)E.cellState(root, r, c);
        CNF F;
        encodePuzzle(p, known, F);
        FILE* f = fopen(opt.dimacs.c_str(), "w");
        fprintf(f, "p cnf %d %ld\nc cells %d %d\n", F.nvars, F.nclauses, p.W, p.H);
        for (int x : F.lits) fprintf(f, x ? "%d " : "0\n", x);
        fclose(f);
        fprintf(stderr, "wrote %s: %d vars %ld clauses, %d/%d cells known\n", opt.dimacs.c_str(), F.nvars,
                F.nclauses, root.known, p.H * p.W);
        return finish(false);
    }
    bool useSat = opt.sat != "none";
    E.nodeLimit = useSat ? opt.dfsNodes : opt.nodeLimit;
    E.probeLimit = useSat ? (long)(opt.dfsWork / (p.H + p.W)) : -1;
    E.trackBest = useSat && opt.satDfsPhase;
    Status ds;
    auto luby = [](long i) {  // 1 1 2 1 1 2 4 ...
        long size = 1, seq = 0;
        while (size < i + 1) {
            seq++;
            size = 2 * size + 1;
        }
        while (size - 1 != i) {
            size = (size - 1) >> 1;
            seq--;
            i = i % size;
        }
        return 1L << seq;
    };
    auto rootInfo = E.pinfo;
    auto rootTouched = E.ptouched;
    auto rootStamp = E.lineStamp;
    long run = 0;
    // Runs restart rounds until a solution, exhaustion, or a budget abort.  A
    // round cut short by the work budget is repeated when called again.
    auto runDfs = [&]() -> Status {
        if (opt.restartBase <= 0) {
            s = root;
            E.pinfo = rootInfo;
            E.ptouched = rootTouched;
            E.lineStamp = rootStamp;
            return E.dfs(s, 0);
        }
        for (;; run++) {
            E.restartBudget = opt.restartBase * luby(run);
            E.runNodes = 0;
            E.restartHit = false;
            E.noise = run < 2 ? 0.0 : opt.noise;
            E.valueOrder = (run % 2 == 0) ? opt.valueOrder : (opt.valueOrder == 0 ? 1 : 0);
            E.pinfo = rootInfo;
            E.ptouched = rootTouched;
            E.lineStamp = rootStamp;
            s = root;
            Status r = E.dfs(s, 0);
            if (r == S_ABORT && E.restartHit && !E.aborted) continue;
            return r;
        }
    };
    std::vector<int8_t> known(p.H * p.W, -1);
    for (int r = 0; r < p.H; r++)
        for (int c = 0; c < p.W; c++) known[r * p.W + c] = (int8_t)E.cellState(root, r, c);
    if (useSat && opt.maxSolutions == 1 && opt.interleave) {
        // Alternate DFS and a resumable CDCL run with growing budgets.
        const int L = p.H + p.W;
        double work = opt.dfsWork1;
        long conflicts = opt.satConflicts1;
        E.nodeLimit = -1;
        E.probeLimit = (long)(work / L);
        IncSat* S = nullptr;
        for (;;) {
            E.aborted = false;
            ds = runDfs();
            if (ds != S_ABORT || !E.solutions.empty()) break;
            if (now_sec() > deadline) break;
            res.usedSat = true;
            if (!S) S = incsatCreate(p, known, deadline);
            if (!S) {
                ds = S_CONFLICT;
                break;
            }
            int r = incsatSolve(S, conflicts);
            if (r == 10) {
                std::vector<uint8_t> g;
                incsatModel(S, g);
                E.solutions.push_back(g);
                break;
            }
            if (r == 20) {
                ds = S_CONFLICT;
                break;
            }
            if (now_sec() > deadline) break;
            work *= 2;
            conflicts *= 4;
            E.probeLimit = E.st.probes + (long)(work / L);
        }
        if (S) incsatDestroy(S);
        bool used = res.usedSat;
        Result out = finish(!E.solutions.empty() || ds == S_CONFLICT);
        out.usedSat = used;
        return out;
    }
    ds = runDfs();
    if (ds != S_ABORT || (int)E.solutions.size() >= opt.maxSolutions) return finish(true);
    if (!useSat || now_sec() > deadline) return finish(false);
    // CDCL fallback from the root-probed state
    res.usedSat = true;
    std::vector<std::vector<uint8_t>> found = E.solutions;
    std::vector<int> extra;
    if (E.trackBest && E.best.known > root.known) {
        g_hintKnown.assign(p.H * p.W, -1);
        for (int r = 0; r < p.H; r++)
            for (int c = 0; c < p.W; c++) g_hintKnown[r * p.W + c] = (int8_t)E.cellState(E.best, r, c);
    }
    if (opt.probeClauses) {
        double ti = now_sec();
        E.implications(root, extra, (size_t)opt.probeClauseLits);
        if (opt.verbose) fprintf(stderr, "[sat] probe clauses: %zu literals (%.3fs)\n", extra.size(), now_sec() - ti);
    }
    std::string backend = opt.sat;
    if (backend == "auto") backend = opt.maxSolutions == 1 ? "hybrid" : "kissat";
    SatResult sr = satSearch(p, known, found, opt.maxSolutions, deadline, opt.verbose, backend,
                             opt.probeClauses ? &extra : nullptr);
    E.solutions = found;
    E.st.nodes = E.st.nodes;  // keep counters
    Result out = finish(sr != SAT_TIMEOUT);
    out.usedSat = true;
    return out;
}

static Result solvePuzzle(const Puzzle& p, const Options& opt) {
    std::string why;
    if (!p.sane(&why)) {
        Result r;
        r.status = R_NONE;
        return r;
    }
    int m = std::max(p.H, p.W) + 2;
    if (m <= 64) return solveT<1>(p, opt);
    if (m <= 128) return solveT<2>(p, opt);
    if (m <= 192) return solveT<3>(p, opt);
    if (m <= 256) return solveT<4>(p, opt);
    if (m <= 512) return solveT<8>(p, opt);
    Result r;
    r.status = R_ERROR;
    return r;
}

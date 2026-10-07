// CDCL solution search: Kissat for the first solution, then a cheap
// neighbourhood check and incremental, solution-guided CaDiCaL for more.
#include "sat.h"

#include <cstdio>
#include <set>
#include <tuple>

#include "cadical.hpp"
#include "engine.h"
extern "C" {
#include "kissat.h"
}

namespace {

struct DeadlineTerminator : CaDiCaL::Terminator {
    double deadline;
    long calls = 0;
    bool terminate() override {
        if ((++calls & 63) != 0) return false;
        return now_sec() > deadline;
    }
};

int kissatTerminate(void* state) {
    static long calls = 0;
    if ((++calls & 63) != 0) return 0;
    return now_sec() > *(double*)state;
}

// Look for a second solution that differs from g by a single 2x2 swap.
bool swapNeighbour(const Puzzle& p, const std::vector<uint8_t>& g, std::vector<uint8_t>& out) {
    const int H = p.H, W = p.W;
    // Valid moves of a single black cell inside one line that keep its runs:
    // shifting a block by one, or moving a length-1 block within its gap.
    auto lineMoves = [](const std::vector<int>& v, std::vector<std::pair<int, int>>& mv) {
        int n = (int)v.size();
        mv.clear();
        std::vector<std::pair<int, int>> blocks;
        for (int i = 0; i < n;) {
            if (!v[i]) {
                i++;
                continue;
            }
            int j = i;
            while (j < n && v[j]) j++;
            blocks.push_back({i, j - 1});
            i = j;
        }
        for (size_t b = 0; b < blocks.size(); b++) {
            int a = blocks[b].first, e = blocks[b].second;
            int lo = b ? blocks[b - 1].second + 2 : 0;           // first cell allowed for this block
            int hi = b + 1 < blocks.size() ? blocks[b + 1].first - 2 : n - 1;
            if (e + 1 <= hi) mv.push_back({a, e + 1});           // shift right
            if (a - 1 >= lo) mv.push_back({e, a - 1});           // shift left
            if (a == e)
                for (int c = lo; c <= hi; c++)
                    if (c != a && c != a - 1 && c != a + 1) mv.push_back({a, c});
        }
    };
    std::set<std::tuple<int, int, int>> rowMv, colMv;
    std::vector<int> line;
    std::vector<std::pair<int, int>> mv;
    for (int r = 0; r < H; r++) {
        line.assign(g.begin() + r * W, g.begin() + (r + 1) * W);
        lineMoves(line, mv);
        for (auto& m : mv) rowMv.insert({r, m.first, m.second});
    }
    for (int c = 0; c < W; c++) {
        line.clear();
        for (int r = 0; r < H; r++) line.push_back(g[r * W + c]);
        lineMoves(line, mv);
        for (auto& m : mv) colMv.insert({c, m.first, m.second});
    }
    for (auto& m : rowMv) {
        int r1, c1, c2;
        std::tie(r1, c1, c2) = m;
        // column c1 must move its black cell from r1 to some r2
        for (auto it = colMv.lower_bound({c1, r1, -1}); it != colMv.end(); ++it) {
            if (std::get<0>(*it) != c1 || std::get<1>(*it) != r1) break;
            int r2 = std::get<2>(*it);
            if (!rowMv.count({r2, c2, c1})) continue;
            if (!colMv.count({c2, r2, r1})) continue;
            out = g;
            out[r1 * W + c1] = 0;
            out[r1 * W + c2] = 1;
            out[r2 * W + c2] = 0;
            out[r2 * W + c1] = 1;
            return true;
        }
    }
    return false;
}

}  // namespace

SatResult satSearch(const Puzzle& p, const std::vector<int8_t>& known,
                    std::vector<std::vector<uint8_t>>& found, int want, double deadline,
                    bool verbose, const std::string& backend, const std::vector<int>* extra) {
    CNF F;
    double t0 = now_sec();
    if (!encodePuzzle(p, known, F)) return SAT_UNSAT;
    if (extra) {
        for (int x : *extra) {
            F.lits.push_back(x);
            if (!x) F.nclauses++;
        }
    }
    const int N = p.H * p.W;
    if (verbose)
        fprintf(stderr, "[sat] vars=%d clauses=%ld encode=%.3fs\n", F.nvars, F.nclauses, now_sec() - t0);
    auto blockingClause = [&](const std::vector<uint8_t>& g, std::vector<int>& cl) {
        cl.clear();
        for (int i = 0; i < N; i++)
            if (known[i] < 0) cl.push_back(g[i] ? -(i + 1) : (i + 1));
    };
    std::vector<int> cl;

    if (found.empty() && backend == "kissat") {
        kissat* K = kissat_init();
        if (!g_kissatConfig.empty()) kissat_set_configuration(K, g_kissatConfig.c_str());
        kissat_set_option(K, "quiet", 1);
        if (g_seed) kissat_set_option(K, "seed", g_seed);
        kissat_reserve(K, F.nvars);
        for (int x : F.lits) kissat_add(K, x);
        double dl = deadline;
        kissat_set_terminate(K, &dl, kissatTerminate);
        int r = kissat_solve(K);
        if (r == 10) {
            std::vector<uint8_t> g(N);
            for (int i = 0; i < N; i++) g[i] = kissat_value(K, i + 1) > 0;
            found.push_back(g);
            if (verbose) fprintf(stderr, "[sat] kissat solution 1 at %.3fs\n", now_sec() - t0);
        }
        kissat_release(K);
        if (r == 20) return SAT_UNSAT;
        if (r != 10) return SAT_TIMEOUT;
    }

    CaDiCaL::Solver* S = nullptr;
    DeadlineTerminator term;
    term.deadline = deadline;
    while ((int)found.size() < want) {
        if (!found.empty()) {
            std::vector<uint8_t> alt;
            bool got = false;
            for (auto& g : found) {
                if (swapNeighbour(p, g, alt)) {
                    bool dup = false;
                    for (auto& h : found) dup |= h == alt;
                    if (!dup) {
                        got = true;
                        break;
                    }
                }
            }
            if (got) {
                found.push_back(alt);
                if (verbose) fprintf(stderr, "[sat] swap solution %zu at %.3fs\n", found.size(), now_sec() - t0);
                continue;
            }
        }
        if (!S) {
            S = new CaDiCaL::Solver;
            S->set("quiet", 1);
            if (g_seed) S->set("seed", g_seed);
            for (int x : F.lits) S->add(x);
            for (auto& g : found) {
                blockingClause(g, cl);
                for (int x : cl) S->add(x);
                S->add(0);
            }
            S->connect_terminator(&term);
        }
        if (!found.empty()) {
            const auto& g = found.back();
            for (int i = 0; i < N; i++) S->phase(g[i] ? i + 1 : -(i + 1));
        }
        // Solution-guided CaDiCaL finds nearby solutions quickly; if it does
        // not within a conflict budget, Kissat is usually faster at proving
        // that no further solution exists.
        bool hybrid = backend == "kissat" && !found.empty() && g_cadicalConflicts > 0;
        if (hybrid) S->limit("conflicts", g_cadicalConflicts);
        int r = S->solve();
        if (r == 0 && hybrid && now_sec() < deadline) {
            if (verbose) fprintf(stderr, "[sat] cadical budget exhausted at %.3fs, switching to kissat\n", now_sec() - t0);
            kissat* K = kissat_init();
            kissat_set_option(K, "quiet", 1);
            if (g_seed) kissat_set_option(K, "seed", g_seed);
            kissat_reserve(K, F.nvars);
            for (int x : F.lits) kissat_add(K, x);
            for (auto& g : found) {
                blockingClause(g, cl);
                for (int x : cl) kissat_add(K, x);
                kissat_add(K, 0);
            }
            double dl = deadline;
            kissat_set_terminate(K, &dl, kissatTerminate);
            r = kissat_solve(K);
            if (r == 10) {
                std::vector<uint8_t> g(N);
                for (int i = 0; i < N; i++) g[i] = kissat_value(K, i + 1) > 0;
                found.push_back(g);
                if (verbose) fprintf(stderr, "[sat] kissat solution %zu at %.3fs\n", found.size(), now_sec() - t0);
                blockingClause(g, cl);
                for (int x : cl) S->add(x);
                S->add(0);
                kissat_release(K);
                continue;
            }
            kissat_release(K);
        }
        if (r == 10) {
            std::vector<uint8_t> g(N);
            for (int i = 0; i < N; i++) g[i] = S->val(i + 1) > 0;
            found.push_back(g);
            if (verbose) fprintf(stderr, "[sat] cadical solution %zu at %.3fs\n", found.size(), now_sec() - t0);
            blockingClause(g, cl);
            for (int x : cl) S->add(x);
            S->add(0);
        } else {
            if (verbose && r == 20) fprintf(stderr, "[sat] no further solution (%.3fs)\n", now_sec() - t0);
            delete S;
            return r == 20 ? SAT_UNSAT : SAT_TIMEOUT;
        }
    }
    delete S;
    return SAT_DONE;
}

struct IncSat {
    CaDiCaL::Solver solver;
    DeadlineTerminator term;
    int N = 0;
};

IncSat* incsatCreate(const Puzzle& p, const std::vector<int8_t>& known, double deadline) {
    CNF F;
    if (!encodePuzzle(p, known, F)) return nullptr;
    IncSat* S = new IncSat;
    S->N = p.H * p.W;
    S->solver.set("quiet", 1);
    if (g_seed) S->solver.set("seed", g_seed);
    for (int x : F.lits) S->solver.add(x);
    S->term.deadline = deadline;
    S->solver.connect_terminator(&S->term);
    return S;
}

int incsatSolve(IncSat* S, long conflicts) {
    if (conflicts >= 0) S->solver.limit("conflicts", (int)std::min<long>(conflicts, 1L << 30));
    return S->solver.solve();
}

void incsatModel(IncSat* S, std::vector<uint8_t>& g) {
    g.assign(S->N, 0);
    for (int i = 0; i < S->N; i++) g[i] = S->solver.val(i + 1) > 0;
}

void incsatDestroy(IncSat* S) { delete S; }

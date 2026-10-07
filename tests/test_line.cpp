// Randomized check of LineSolver against brute-force enumeration.
#include "../src/linesolve.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>

static bool consistent(const std::vector<int>& cells, int n, const std::vector<int>& clue) {
    std::vector<int> runs; int r = 0;
    for (int i = 0; i < n; i++) { if (cells[i]) r++; else { if (r) runs.push_back(r); r = 0; } }
    if (r) runs.push_back(r);
    return runs == clue;
}

template <int NW>
int run(int iters, int maxn, unsigned seed) {
    std::mt19937 rng(seed);
    int bad = 0;
    for (int it = 0; it < iters; it++) {
        int n = 1 + rng() % maxn;
        // random solution -> clue, then random partial knowledge (maybe inconsistent)
        std::vector<int> sol(n);
        int dens = rng() % 100;
        for (int i = 0; i < n; i++) sol[i] = (int)(rng() % 100) < dens;
        std::vector<int> clue; int r = 0;
        for (int i = 0; i < n; i++) { if (sol[i]) r++; else { if (r) clue.push_back(r); r = 0; } }
        if (r) clue.push_back(r);
        if (rng() % 4 == 0 && !clue.empty()) clue[rng() % clue.size()] += (rng() % 3) - 1;
        std::vector<int> cl2; for (int x : clue) if (x > 0) cl2.push_back(x);
        clue = cl2;
        Bits<NW> B = Bits<NW>::zero(), W = Bits<NW>::zero();
        int kn = rng() % 100;
        for (int i = 0; i < n; i++) if ((int)(rng() % 100) < kn) {
            int v = (rng() % 10 == 0) ? !sol[i] : sol[i];
            if (v) B.set(i); else W.set(i);
        }
        // brute force (only for n <= 20)
        if (n > 14) continue;
        std::vector<int> canB(n, 0), canW(n, 0); bool any = false;
        std::vector<int> cells(n);
        for (long m = 0; m < (1L << n); m++) {
            bool ok = true;
            for (int i = 0; i < n; i++) { cells[i] = (m >> i) & 1; if ((cells[i] && W.test(i)) || (!cells[i] && B.test(i))) { ok = false; break; } }
            if (!ok || !consistent(cells, n, clue)) continue;
            any = true;
            for (int i = 0; i < n; i++) { if (cells[i]) canB[i] = 1; else canW[i] = 1; }
        }
        Bits<NW> oB, oW;
        bool res = LineSolver<NW>::solve(n, clue.data(), (int)clue.size(), B, W, oB, oW);
        bool mism = res != any;
        if (!mism && res) for (int i = 0; i < n; i++) {
            if (oB.test(i) != (canB[i] && !canW[i])) mism = true;
            if (oW.test(i) != (canW[i] && !canB[i])) mism = true;
        }
        if (!mism && res) { if (oB.shr(n).any() || oW.shr(n).any()) mism = true; }
        if (mism) {
            if (bad < 10) {
                printf("MISMATCH n=%d clue=", n); for (int x : clue) printf("%d ", x);
                printf(" known="); for (int i = 0; i < n; i++) printf("%c", B.test(i) ? '#' : W.test(i) ? '.' : '?');
                printf(" brute=%d solver=%d\n", any, res);
                if (any && res) { printf("  exp: "); for (int i = 0; i < n; i++) printf("%c", canB[i] && !canW[i] ? '#' : canW[i] && !canB[i] ? '.' : '?');
                  printf("\n  got: "); for (int i = 0; i < n; i++) printf("%c", oB.test(i) ? '#' : oW.test(i) ? '.' : '?'); printf("\n"); }
            }
            bad++;
        }
    }
    return bad;
}

int main() {
    int b1 = run<1>(300000, 14, 1);
    int b2 = run<2>(100000, 14, 2);
    int b3 = run<3>(50000, 14, 3);
    printf("bad: NW1=%d NW2=%d NW3=%d\n", b1, b2, b3);
    return (b1 || b2 || b3) ? 1 : 0;
}

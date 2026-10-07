// Check multi-word LineSolver against a scalar O(n*k) DP on long lines.
#include "../src/linesolve.h"
#include <cstdio>
#include <vector>
#include <random>
using namespace std;

// returns false if infeasible; fills canB/canW
static bool scalar(int n, const vector<int>& c, const vector<int>& known, vector<int>& canB, vector<int>& canW) {
    int k = c.size();
    // f[i][j]: cells [0,i) explained by blocks 0..j-1, and if j>0 the last block ended at <= i-1 with
    // a white separator already included when i < n... use standard formulation with explicit sep.
    auto noW = [&](int a, int b) { for (int i = a; i < b; i++) if (known[i] == 0) return false; return true; };
    vector<vector<char>> f(n + 2, vector<char>(k + 1, 0)), g(n + 2, vector<char>(k + 1, 0));
    f[0][0] = 1;
    for (int i = 0; i <= n; i++) for (int j = 0; j <= k; j++) if (f[i][j]) {
        if (i < n && known[i] != 1) f[i + 1][j] = 1;  // white cell
        if (j < k && i + c[j] <= n && noW(i, i + c[j])) {
            int e = i + c[j];
            if (e == n) f[n + 1][j + 1] = 1;
            else if (known[e] != 1) f[e + 1][j + 1] = 1;
        }
    }
    bool ok = f[n][k] || f[n + 1][k];
    if (!ok) return false;
    // g[i][j]: cells [i,n) explained by blocks j..k-1 (i may be n+1 meaning nothing left)
    g[n][k] = 1; g[n + 1][k] = 1;
    for (int i = n - 1; i >= 0; i--) for (int j = k; j >= 0; j--) {
        bool v = false;
        if (known[i] != 1 && g[i + 1][j]) v = true;
        if (j < k && i + c[j] <= n && noW(i, i + c[j])) {
            int e = i + c[j];
            if (e == n) { if (g[n + 1][j + 1]) v = true; }
            else if (known[e] != 1 && g[e + 1][j + 1]) v = true;
        }
        g[i][j] = v;
    }
    canB.assign(n, 0); canW.assign(n, 0);
    for (int i = 0; i < n; i++) for (int j = 0; j <= k; j++) {
        if (f[i][j] && known[i] != 1 && g[i + 1][j]) canW[i] = 1;
        if (j < k && f[i][j] && i + c[j] <= n && noW(i, i + c[j])) {
            int e = i + c[j]; bool suf;
            if (e == n) suf = g[n + 1][j + 1]; else suf = known[e] != 1 && g[e + 1][j + 1];
            if (suf) { for (int t = i; t < e; t++) canB[t] = 1; if (e < n) canW[e] = 1; }
        }
    }
    return true;
}

template <int NW>
int run(int iters, int minn, int maxn, unsigned seed) {
    mt19937 rng(seed); int bad = 0;
    for (int it = 0; it < iters; it++) {
        int n = minn + rng() % (maxn - minn + 1);
        vector<int> sol(n); int dens = 20 + rng() % 70;
        for (int i = 0; i < n; i++) sol[i] = (int)(rng() % 100) < dens;
        vector<int> clue; int r = 0;
        for (int i = 0; i < n; i++) { if (sol[i]) r++; else { if (r) clue.push_back(r); r = 0; } }
        if (r) clue.push_back(r);
        if (rng() % 5 == 0 && !clue.empty()) { int idx = rng() % clue.size(); clue[idx] += (int)(rng() % 3) - 1; if (clue[idx] <= 0) clue.erase(clue.begin() + idx); }
        vector<int> known(n, -1); Bits<NW> B = Bits<NW>::zero(), W = Bits<NW>::zero();
        int kn = rng() % 90;
        for (int i = 0; i < n; i++) if ((int)(rng() % 100) < kn) { int v = (rng() % 30 == 0) ? !sol[i] : sol[i]; known[i] = v; if (v) B.set(i); else W.set(i); }
        vector<int> cb, cw; bool e = scalar(n, clue, known, cb, cw);
        Bits<NW> oB, oW; bool g = LineSolver<NW>::solve(n, clue.data(), clue.size(), B, W, oB, oW);
        bool mism = e != g;
        if (!mism && e) for (int i = 0; i < n; i++) { if (oB.test(i) != (cb[i] && !cw[i]) || oW.test(i) != (cw[i] && !cb[i])) mism = true; }
        if (mism) { if (bad < 5) printf("MISMATCH NW=%d n=%d exp=%d got=%d\n", NW, n, e, g); bad++; }
    }
    return bad;
}
int main() {
    int b = 0;
    b += run<1>(200000, 1, 62, 11);
    b += run<2>(200000, 40, 126, 12);
    b += run<3>(100000, 100, 190, 13);
    b += run<4>(50000, 150, 254, 14);
    printf("bad=%d\n", b);
    return b != 0;
}

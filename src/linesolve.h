// Bit-parallel complete line solver.
//
// For a line of length n with clue c[0..k-1] and known cells (B = known black,
// W = known white), computes the set of cells that are black in every
// completion and the set that are white in every completion (generalized arc
// consistency for the line constraint), or reports a conflict.
//
// Positions are bit indices.  Cell n is treated as a virtual white cell.
// Forward pass, for j = 0..k:
//   P[j]  : "free" positions t: cells [0,t) are explained by blocks 0..j-1,
//           the last explained cell being white (or t == 0 when j == 0).
//   R[j]  : positions reachable from P[j] moving right through non-black cells.
//   St[j] : feasible start positions of block j w.r.t. the prefix.
// The backward pass is the forward pass on the mirrored line.
// All work is O(k * log(maxclue)) word operations.
#pragma once
#include "bits.h"

template <int NW>
struct LineSolver {
    using B_t = Bits<NW>;
    static constexpr int MAXK = 64 * NW / 2 + 2;

    // fillRight(S, M): S plus every s such that s-1 is in the result and cell
    // s-1 is in M.  Uses the carry trick: runs of M are swept by an addition.
    static inline B_t fillRight(const B_t& S, const B_t& M) {
        B_t E = S & M;
        return (M.add(E) ^ M) | S;
    }
    // positions s such that cells [s, s+c) are all in NWm
    static inline B_t runs(const B_t& NWm, int c) {
        B_t r = NWm;
        int len = 1;
        while (2 * len <= c) {
            r = r & r.shr(len);
            len *= 2;
        }
        if (len < c) r = r & r.shr(c - len);
        return r;
    }
    static inline B_t spread(const B_t& V, int c) {
        B_t r = V;
        int len = 1;
        while (2 * len <= c) {
            r = r | r.shl(len);
            len *= 2;
        }
        if (len < c) r = r | r.shl(c - len);
        return r;
    }

    // Returns false if no placement exists.  St/P/R arrays have k+1 entries.
    static inline bool forward(int n, const int* c, int k, const B_t& B, const B_t& W, B_t* St,
                               B_t* P, B_t* R) {
        const B_t NB = ~B;
        const B_t NWm = (~W) & B_t::ones(n);
        const B_t lim = B_t::ones(n + 2);
        P[0] = B_t::bit(0);
        R[0] = fillRight(P[0], NB) & lim;
        for (int j = 0; j < k; j++) {
            int cj = c[j];
            St[j] = R[j] & runs(NWm, cj) & NB.shr(cj);
            if (St[j].none()) return false;
            P[j + 1] = St[j].shl(cj + 1);
            R[j + 1] = fillRight(P[j + 1], NB) & lim;
        }
        return R[k].test(n + 1);
    }

    // Main entry.  outB/outW receive all cells forced black/white (including
    // already known ones).  Returns false on conflict.
    static bool solve(int n, const int* c, int k, const B_t& B, const B_t& W, B_t& outB,
                      B_t& outW) {
        B_t St[MAXK], P[MAXK], R[MAXK];
        B_t St2[MAXK], P2[MAXK], R2[MAXK];
        int rc[MAXK];
        if (!forward(n, c, k, B, W, St, P, R)) return false;
        for (int j = 0; j < k; j++) rc[j] = c[k - 1 - j];
        const B_t cellsN = B_t::ones(n);
        const B_t lim1 = B_t::ones(n + 1);
        B_t Bm = B.mirror(n).shr(1), Wm = W.mirror(n).shr(1);
        if (!forward(n, rc, k, Bm, Wm, St2, P2, R2)) return false;  // cannot happen
        const B_t NB = ~B, NBm = ~Bm;
        B_t canB = B_t::zero(), canW = B_t::zero();
        for (int j = 0; j < k; j++) {
            B_t T = (R2[k - 1 - j] & lim1).mirror(n);
            B_t V = St[j] & T.shr(c[j]);
            canB |= spread(V, c[j]);
        }
        for (int j = 0; j <= k; j++) {
            B_t G = (j == 0 ? R[0] : (P[j].shr(1) | R[j])) & NB & cellsN;
            int m = k - j;
            B_t G2 = (m == 0 ? R2[0] : (P2[m].shr(1) | R2[m])) & NBm & cellsN;
            B_t H = G2.mirror(n).shr(1);
            canW |= G & H;
        }
        canB &= cellsN;
        canW &= cellsN;
        outB = cellsN.andnot(canW);
        outW = cellsN.andnot(canB);
        if ((outB & outW).any()) return false;
        if ((outB & W).any() || (outW & B).any()) return false;
        return true;
    }
};

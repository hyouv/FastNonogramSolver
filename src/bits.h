// Fixed-width multi-word bitsets used for line masks.
#pragma once
#include <cstdint>
#include <cstring>

static inline uint64_t rbit64(uint64_t x) {
#if defined(__has_builtin)
#if __has_builtin(__builtin_bitreverse64)
    return __builtin_bitreverse64(x);
#endif
#endif
    x = ((x >> 1) & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
    x = ((x >> 2) & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
    x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
    return __builtin_bswap64(x);
}

template <int NW>
struct Bits {
    uint64_t w[NW];

    static constexpr int NBITS = 64 * NW;

    static inline Bits zero() {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = 0;
        return r;
    }
    static inline Bits all() {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = ~0ULL;
        return r;
    }
    // bits [0, n)
    static inline Bits ones(int n) {
        Bits r;
        for (int i = 0; i < NW; i++) {
            int lo = 64 * i;
            if (n >= lo + 64) r.w[i] = ~0ULL;
            else if (n <= lo) r.w[i] = 0;
            else r.w[i] = (1ULL << (n - lo)) - 1;
        }
        return r;
    }
    static inline Bits bit(int i) {
        Bits r = zero();
        r.w[i >> 6] = 1ULL << (i & 63);
        return r;
    }
    inline bool test(int i) const { return (w[i >> 6] >> (i & 63)) & 1; }
    inline void set(int i) { w[i >> 6] |= 1ULL << (i & 63); }
    inline void clr(int i) { w[i >> 6] &= ~(1ULL << (i & 63)); }

    inline Bits operator&(const Bits& o) const {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = w[i] & o.w[i];
        return r;
    }
    inline Bits operator|(const Bits& o) const {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = w[i] | o.w[i];
        return r;
    }
    inline Bits operator^(const Bits& o) const {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = w[i] ^ o.w[i];
        return r;
    }
    inline Bits operator~() const {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = ~w[i];
        return r;
    }
    inline Bits& operator&=(const Bits& o) {
        for (int i = 0; i < NW; i++) w[i] &= o.w[i];
        return *this;
    }
    inline Bits& operator|=(const Bits& o) {
        for (int i = 0; i < NW; i++) w[i] |= o.w[i];
        return *this;
    }
    inline Bits andnot(const Bits& o) const {  // this & ~o
        Bits r;
        for (int i = 0; i < NW; i++) r.w[i] = w[i] & ~o.w[i];
        return r;
    }
    inline bool operator==(const Bits& o) const {
        for (int i = 0; i < NW; i++)
            if (w[i] != o.w[i]) return false;
        return true;
    }
    inline bool operator!=(const Bits& o) const { return !(*this == o); }
    inline bool any() const {
        uint64_t a = 0;
        for (int i = 0; i < NW; i++) a |= w[i];
        return a != 0;
    }
    inline bool none() const { return !any(); }
    inline int popcount() const {
        int c = 0;
        for (int i = 0; i < NW; i++) c += __builtin_popcountll(w[i]);
        return c;
    }
    // lowest set bit index, -1 if none
    inline int lowest() const {
        for (int i = 0; i < NW; i++)
            if (w[i]) return 64 * i + __builtin_ctzll(w[i]);
        return -1;
    }

    inline Bits shl(int k) const {
        if (NW == 1) {
            Bits r;
            r.w[0] = k >= 64 ? 0 : (w[0] << k);
            return r;
        }
        Bits r = zero();
        int ws = k >> 6, bs = k & 63;
        for (int i = NW - 1; i >= ws; i--) {
            uint64_t v = w[i - ws] << bs;
            if (bs && i - ws - 1 >= 0) v |= w[i - ws - 1] >> (64 - bs);
            r.w[i] = v;
        }
        return r;
    }
    inline Bits shr(int k) const {
        if (NW == 1) {
            Bits r;
            r.w[0] = k >= 64 ? 0 : (w[0] >> k);
            return r;
        }
        Bits r = zero();
        int ws = k >> 6, bs = k & 63;
        for (int i = 0; i + ws < NW; i++) {
            uint64_t v = w[i + ws] >> bs;
            if (bs && i + ws + 1 < NW) v |= w[i + ws + 1] << (64 - bs);
            r.w[i] = v;
        }
        return r;
    }
    inline Bits add(const Bits& o) const {
        Bits r;
        uint64_t carry = 0;
        for (int i = 0; i < NW; i++) {
            uint64_t s = w[i] + o.w[i];
            uint64_t c1 = s < w[i];
            uint64_t s2 = s + carry;
            uint64_t c2 = s2 < s;
            r.w[i] = s2;
            carry = c1 | c2;
        }
        return r;
    }
    // Maps bit t -> bit (n - t) for t in [0, n]; requires bits only in [0, n].
    inline Bits mirror(int n) const {
        Bits r;
        for (int i = 0; i < NW; i++) r.w[NW - 1 - i] = rbit64(w[i]);
        return r.shr(NBITS - 1 - n);
    }
    template <class F>
    inline void foreach_bit(F f) const {
        for (int i = 0; i < NW; i++) {
            uint64_t x = w[i];
            while (x) {
                int b = __builtin_ctzll(x);
                f(64 * i + b);
                x &= x - 1;
            }
        }
    }
};

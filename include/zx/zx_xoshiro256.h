#ifndef ZX_XOSHIRO256_H
#define ZX_XOSHIRO256_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>

// this file implements Xoshiro256++ algorithm for random number generation.

struct ZXXoshiro256 {
  zx_u64 s[4];

  static inline ZXXoshiro256 init(zx_u64 seed) {
    constexpr zx_u64 PHI = 0x9e3779b97f4a7c15;

    zx_u64 state = seed;
    ZXXoshiro256 rand;

    for (zx_u8 i = 0; i < 4; i++) {
      state += PHI;
      zx_u64 z = state;
      z = (z ^ (z >> 30)) * (zx_u64){0xbf58476d1ce4e5ba},
      z = (z ^ (z >> 27)) * (zx_u64){0x94d049bb133111eb};
      z = z ^ (z >> 31);
      rand.s[i] = z;
    }

    ZX_DBG_ASSERT(((rand.s[0] != 0) || (rand.s[1] != 0) || (rand.s[2] != 0) ||
                   (rand.s[3] != 0)));

    return rand;
  }

  __attribute__((always_inline)) inline zx_u64 next() {
    zx_u64 const r =
        __builtin_rotateleft64(this->s[0] + this->s[3], 23) + this->s[0];
    zx_u64 const t = this->s[1] << 17;

    this->s[2] ^= this->s[0];
    this->s[3] ^= this->s[1];
    this->s[1] ^= this->s[2];
    this->s[0] ^= this->s[3];

    this->s[2] ^= t;
    this->s[3] = __builtin_rotateleft64(this->s[3], 45);

    return r;
  }

  inline void fill(zx_u8 *__restrict out, zx_u64 len) {
    if (len == 0) {
      return;
    }

    ZX_ASSERT((out != nullptr));

    zx_u64 const n_blocks = len / sizeof(zx_u64);

    for (zx_u64 i = 0; i < n_blocks; i++) {
      zx_u64 const next = this->next();
      __builtin_memcpy(&out[i * sizeof(zx_u64)], &next, sizeof(zx_u64));
    }

    zx_u64 const blocks_end = sizeof(zx_u64) * n_blocks;
    if (blocks_end < len) {
      zx_u64 const next = this->next();
      __builtin_memcpy(&out[blocks_end], &next, len - blocks_end);
    }
  }
};

#endif

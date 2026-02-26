#ifndef ZX_XOSHIRO256_H
#define ZX_XOSHIRO256_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>

// this file implements Xoshiro256++ algorithm for random number generation.

typedef struct ZXXoshiro256 {
  zx_u64 s[4];
} ZXXoshiro256;

static inline ZXXoshiro256 ZXXoshiro256_init(zx_u64 seed) {
  constexpr zx_u64 PHI = 0x9e3779b97f4a7c15wbu;

  zx_u64 state = seed;
  ZXXoshiro256 rand;

  for (zx_usize i = 0; i < 4; i++) {
    state += PHI;
    zx_u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5bawbu,
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebwbu;
    z = z ^ (z >> 31);
    rand.s[i] = z;
  }

  ZX_DBG_ASSERT(((rand.s[0] != 0) || (rand.s[1] != 0) || (rand.s[2] != 0) ||
                 (rand.s[3] != 0)));

  return rand;
}

static inline zx_u64 ZXXoshiro256_next(ZXXoshiro256 *restrict rand) {
  ZX_DBG_ASSERT((rand != nullptr));

  zx_u64 const r =
      __builtin_rotateleft64(rand->s[0] + rand->s[3], 23) + rand->s[0];
  zx_u64 const t = rand->s[1] << 17;

  rand->s[2] ^= rand->s[0];
  rand->s[3] ^= rand->s[1];
  rand->s[1] ^= rand->s[2];
  rand->s[0] ^= rand->s[3];

  rand->s[2] ^= t;
  rand->s[3] = __builtin_rotateleft64(rand->s[3], 45);

  return r;
}

static inline void ZXXoshiro256_fill(ZXXoshiro256 *restrict rand,
                                     zx_u8 *restrict out, zx_usize len) {
  ZX_ASSERT((rand != nullptr));
  ZX_ASSERT((out != nullptr));

  zx_usize const n_blocks = len / sizeof(zx_u64);

  for (zx_usize i = 0; i < n_blocks; i++) {
    zx_u64 const next = ZXXoshiro256_next(rand);
    __builtin_memcpy(&out[i * sizeof(zx_u64)], &next, sizeof(zx_u64));
  }

  zx_usize const blocks_end = sizeof(zx_u64) * n_blocks;
  if (blocks_end < len) {
    zx_u64 const next = ZXXoshiro256_next(rand);
    __builtin_memcpy(&out[blocks_end], &next, len - blocks_end);
  }
}

#endif

#ifndef SHIN_XOSHIRO256_H
#define SHIN_XOSHIRO256_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <string.h>

// this file implements Xoshiro256++ algorithm for random number generation. It
// is taken from rust/zig stdlib implementations.

typedef struct ShinXoshiro256 {
  shin_u64 s[4];
} ShinXoshiro256;

static inline ShinXoshiro256 ShinXoshiro256_init(shin_u64 seed) {
  constexpr shin_u64 PHI = 0x9e3779b97f4a7c15wbu;

  shin_u64 state = seed;
  ShinXoshiro256 rand;

  for (shin_usize i = 0; i < 4; i++) {
    state += PHI;
    shin_u64 z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5bawbu,
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebwbu;
    z = z ^ (z >> 31);
    rand.s[i] = z;
  }

  SHIN_DBG_ASSERT(((rand->s[0] != 0) || (rand->s[1] != 0) ||
                   (rand->s[2] != 0) || (rand->s[3] != 0)));

  return rand;
}

static inline shin_u64 ShinXoshiro256_next(ShinXoshiro256 *restrict rand) {
  shin_u64 const r =
      __builtin_rotateleft64(rand->s[0] + rand->s[3], 23) + rand->s[0];
  shin_u64 const t = rand->s[1] << 17;

  rand->s[2] ^= rand->s[0];
  rand->s[3] ^= rand->s[1];
  rand->s[1] ^= rand->s[2];
  rand->s[0] ^= rand->s[3];

  rand->s[2] ^= t;
  rand->s[3] = __builtin_rotateleft64(rand->s[3], 45);

  return r;
}

static inline void ShinXoshiro256_fill(ShinXoshiro256 *restrict rand,
                                       shin_u8 *restrict out, shin_usize len) {
  shin_usize const n_blocks = len / sizeof(shin_u64);

  for (shin_usize i = 0; i < n_blocks; i++) {
    shin_u64 const next = ShinXoshiro256_next(rand);
    memcpy(&out[i * sizeof(shin_u64)], &next, sizeof(shin_u64));
  }

  shin_usize const blocks_end = sizeof(shin_u64) * n_blocks;
  if (blocks_end < len) {
    shin_u64 const next = ShinXoshiro256_next(rand);
    memcpy(&out[blocks_end], &next, len - blocks_end);
  }
}

#endif

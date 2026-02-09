#ifndef SHIN_XOSHIRO256_H
#define SHIN_XOSHIRO256_H

#include <shin/shin_def.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// this file implements Xoshiro256++ algorithm for random number generation. It
// is taken from rust/zig stdlib implementations.

static inline shin_u64 shin_xoshiro256_splitmix64(shin_u64 *state) {
  *state += 0x9e3779b97f4a7c15wbu;

  shin_u64 z = *state;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9wbu;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebwbu;
  return z ^ (z >> 31);
}

typedef struct ShinXoshiro256 {
  shin_u64 s[4];
} ShinXoshiro256;

static inline ShinXoshiro256 ShinXoshiro256_init(shin_u64 seed) {
  shin_u64 splitmixstate = seed;

  return (ShinXoshiro256){
      .s =
          {
              shin_xoshiro256_splitmix64(&splitmixstate),
              shin_xoshiro256_splitmix64(&splitmixstate),
              shin_xoshiro256_splitmix64(&splitmixstate),
              shin_xoshiro256_splitmix64(&splitmixstate),
          },
  };
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

#ifdef __cplusplus
}
#endif

#endif

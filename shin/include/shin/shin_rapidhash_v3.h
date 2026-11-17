#ifndef SHIN_RAPIDHASH_V3_H
#define SHIN_RAPIDHASH_V3_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <string.h>

// THIS CODE IS PORTED FROM
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/rapidhash.h
// Specifically the "Nano" version, which is optimized for code size and small
// inputs, is used here.

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

static constexpr shin_u64 SHIN_RAPIDHASH_V3_SECRET[8] = {
    0x2d358dccaa6c78a5wbu, 0x8bb84b93962eacc9wbu, 0x4b33a62ed433d4a3wbu,
    0x4d5a2da51de1aa47wbu, 0xa0761d6478bd642fwbu, 0xe7037ed1a0b428dbwbu,
    0x90ed1765281c388cwbu, 0xaaaaaaaaaaaaaaaawbu,
};

static inline void shin_rapidhash_v3_mum(shin_u64 *a, shin_u64 *b) {
  shin_u128 const r = ((shin_u128){*a}) * ((shin_u128){*b});
  *a = (shin_u64)r;
  *b = (shin_u64)(r >> 64);
}

static inline shin_u64 shin_rapidhash_v3_mix(shin_u64 a, shin_u64 b) {
  shin_rapidhash_v3_mum(&a, &b);
  return a ^ b;
}

static inline shin_u64 shin_rapidhash_v3_read64(shin_u8 const *p) {
  shin_u64 v;
  memcpy(&v, p, sizeof(shin_u64));
  return v;
}

static inline shin_u32 shin_rapidhash_v3_read32(shin_u8 const *p) {
  shin_u32 v;
  memcpy(&v, p, sizeof(shin_u32));
  return v;
}

static inline shin_u64 shin_rapidhash_v3_finish(shin_u64 a, shin_u64 b,
                                                shin_u64 seed, shin_u64 i) {
  a ^= SHIN_RAPIDHASH_V3_SECRET[1];
  b ^= seed;
  shin_rapidhash_v3_mum(&a, &b);
  return shin_rapidhash_v3_mix(a ^ SHIN_RAPIDHASH_V3_SECRET[7],
                               b ^ SHIN_RAPIDHASH_V3_SECRET[1] ^ i);
}

static inline shin_u64 shin_rapidhash_v3_16(shin_u8 const *key, shin_u32 len,
                                            shin_u64 seed) {
  SHIN_DBG_ASSERT((len <= 16));

  shin_u64 a = 0;
  shin_u64 b = 0;
  shin_u64 s = seed;

  s ^= shin_rapidhash_v3_mix(s ^ SHIN_RAPIDHASH_V3_SECRET[2],
                             SHIN_RAPIDHASH_V3_SECRET[1]);

  if (len >= 4) {
    s ^= (shin_u64){len};
    if (len >= 8) {
      a = shin_rapidhash_v3_read64(key);
      b = shin_rapidhash_v3_read64(&key[len - 8]);
    } else {
      a = shin_rapidhash_v3_read32(key);
      b = shin_rapidhash_v3_read32(&key[len - 4]);
    }
  } else if (len > 0) {
    a = (((shin_u64){key[0]}) << 45) | key[len - 1];
    b = key[len >> 1];
  } else {
    a = 0;
    b = 0;
  }

  return shin_rapidhash_v3_finish(a, b, s, (shin_u64){len});
}

static inline shin_u64 shin_rapidhash_v3_long(shin_u8 const *key, shin_u32 len,
                                              shin_u64 seed) {
  SHIN_DBG_ASSERT((len > 16));

  shin_u64 a = 0;
  shin_u64 b = 0;
  shin_u64 s = seed;

  shin_u8 const *p = key;
  shin_isize i = len;

  s ^= shin_rapidhash_v3_mix(s ^ SHIN_RAPIDHASH_V3_SECRET[2],
                             SHIN_RAPIDHASH_V3_SECRET[1]);

  if (i > 48) {
    shin_u64 s0 = s;
    shin_u64 s1 = s;

    do {
      s = shin_rapidhash_v3_mix(shin_rapidhash_v3_read64(p) ^
                                    SHIN_RAPIDHASH_V3_SECRET[0],
                                shin_rapidhash_v3_read64(&p[8]) ^ s);

      s0 = shin_rapidhash_v3_mix(shin_rapidhash_v3_read64(&p[16]) ^
                                     SHIN_RAPIDHASH_V3_SECRET[1],
                                 shin_rapidhash_v3_read64(&p[24]) ^ s0);

      s1 = shin_rapidhash_v3_mix(shin_rapidhash_v3_read64(&p[32]) ^
                                     SHIN_RAPIDHASH_V3_SECRET[2],
                                 shin_rapidhash_v3_read64(&p[40]) ^ s1);
      p += 48;
      i -= 48;
    } while (i > 48);

    s ^= s0;
    s ^= s1;
  }

  if (i > 16) {
    s = shin_rapidhash_v3_mix(shin_rapidhash_v3_read64(p) ^
                                  SHIN_RAPIDHASH_V3_SECRET[2],
                              shin_rapidhash_v3_read64(&p[8]) ^ s);
    if (i > 32) {
      s = shin_rapidhash_v3_mix(shin_rapidhash_v3_read64(&p[16]) ^
                                    SHIN_RAPIDHASH_V3_SECRET[2],
                                shin_rapidhash_v3_read64(&p[24]) ^ s);
    }
  }

  _Static_assert(sizeof(i) == sizeof(shin_u64));
  a = shin_rapidhash_v3_read64(&p[i - 16]) ^ ((shin_u64)i);
  b = shin_rapidhash_v3_read64(&p[i - 8]);

  _Static_assert((sizeof(shin_u64) == sizeof(shin_isize)));
  return shin_rapidhash_v3_finish(a, b, s, (shin_u64)i);
}

static inline shin_u64 shin_rapidhash_v3(shin_u8 const *key, shin_u32 len,
                                         shin_u64 seed) {
  if (len <= 16) {
    return shin_rapidhash_v3_16(key, len, seed);
  } else {
    return shin_rapidhash_v3_long(key, len, seed);
  }
}

#endif

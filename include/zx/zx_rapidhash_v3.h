// THIS CODE IS ADAPTED FROM
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/rapidhash.h
// Specifically the "Nano" version, which is optimized for code size and small
// inputs, is used here.
// Original code is licensed under MIT:
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/LICENSE

#ifndef ZX_RAPIDHASH_V3_H
#define ZX_RAPIDHASH_V3_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

static constexpr zx_u64 ZX_RAPIDHASH_V3_SECRET[8] = {
    0x2d358dccaa6c78a5wbu, 0x8bb84b93962eacc9wbu, 0x4b33a62ed433d4a3wbu,
    0x4d5a2da51de1aa47wbu, 0xa0761d6478bd642fwbu, 0xe7037ed1a0b428dbwbu,
    0x90ed1765281c388cwbu, 0xaaaaaaaaaaaaaaaawbu,
};

static inline void zx_rapidhash_v3_mum(zx_u64 *a, zx_u64 *b) {
  zx_u128 const r = ((zx_u128){*a}) * ((zx_u128){*b});
  *a = (zx_u64)r;
  *b = (zx_u64)(r >> 64);
}

static inline zx_u64 zx_rapidhash_v3_mix(zx_u64 a, zx_u64 b) {
  zx_rapidhash_v3_mum(&a, &b);
  return a ^ b;
}

static inline zx_u64 zx_rapidhash_v3_read64(zx_u8 const *p) {
  zx_u64 v;
  __builtin_memcpy(&v, p, sizeof(zx_u64));
  return v;
}

static inline zx_u32 zx_rapidhash_v3_read32(zx_u8 const *p) {
  zx_u32 v;
  __builtin_memcpy(&v, p, sizeof(zx_u32));
  return v;
}

static inline zx_u64 zx_rapidhash_v3_finish(zx_u64 a, zx_u64 b, zx_u64 seed,
                                            zx_u64 i) {
  a ^= ZX_RAPIDHASH_V3_SECRET[1];
  b ^= seed;
  zx_rapidhash_v3_mum(&a, &b);
  return zx_rapidhash_v3_mix(a ^ ZX_RAPIDHASH_V3_SECRET[7],
                             b ^ ZX_RAPIDHASH_V3_SECRET[1] ^ i);
}

static inline zx_u64 zx_rapidhash_v3_16(zx_u8 const *key, zx_u32 len,
                                        zx_u64 seed) {
  ZX_DBG_ASSERT((len <= 16));

  zx_u64 a = 0;
  zx_u64 b = 0;
  zx_u64 s = seed;

  s ^= zx_rapidhash_v3_mix(s ^ ZX_RAPIDHASH_V3_SECRET[2],
                           ZX_RAPIDHASH_V3_SECRET[1]);

  if (len >= 4) {
    s ^= (zx_u64){len};
    if (len >= 8) {
      a = zx_rapidhash_v3_read64(key);
      b = zx_rapidhash_v3_read64(&key[len - 8]);
    } else {
      a = zx_rapidhash_v3_read32(key);
      b = zx_rapidhash_v3_read32(&key[len - 4]);
    }
  } else if (len > 0) {
    a = (((zx_u64){key[0]}) << 45) | key[len - 1];
    b = key[len >> 1];
  } else {
    a = 0;
    b = 0;
  }

  return zx_rapidhash_v3_finish(a, b, s, (zx_u64){len});
}

static inline zx_u64 zx_rapidhash_v3_long(zx_u8 const *key, zx_u32 len,
                                          zx_u64 seed) {
  ZX_DBG_ASSERT((len > 16));

  zx_u64 a = 0;
  zx_u64 b = 0;
  zx_u64 s = seed;

  zx_u8 const *p = key;
  zx_i64 i = len;

  s ^= zx_rapidhash_v3_mix(s ^ ZX_RAPIDHASH_V3_SECRET[2],
                           ZX_RAPIDHASH_V3_SECRET[1]);

  if (i > 48) {
    zx_u64 s0 = s;
    zx_u64 s1 = s;

    do {
      s = zx_rapidhash_v3_mix(zx_rapidhash_v3_read64(p) ^
                                  ZX_RAPIDHASH_V3_SECRET[0],
                              zx_rapidhash_v3_read64(&p[8]) ^ s);

      s0 = zx_rapidhash_v3_mix(zx_rapidhash_v3_read64(&p[16]) ^
                                   ZX_RAPIDHASH_V3_SECRET[1],
                               zx_rapidhash_v3_read64(&p[24]) ^ s0);

      s1 = zx_rapidhash_v3_mix(zx_rapidhash_v3_read64(&p[32]) ^
                                   ZX_RAPIDHASH_V3_SECRET[2],
                               zx_rapidhash_v3_read64(&p[40]) ^ s1);
      p += 48;
      i -= 48;
    } while (i > 48);

    s ^= s0;
    s ^= s1;
  }

  if (i > 16) {
    s = zx_rapidhash_v3_mix(zx_rapidhash_v3_read64(p) ^
                                ZX_RAPIDHASH_V3_SECRET[2],
                            zx_rapidhash_v3_read64(&p[8]) ^ s);
    if (i > 32) {
      s = zx_rapidhash_v3_mix(zx_rapidhash_v3_read64(&p[16]) ^
                                  ZX_RAPIDHASH_V3_SECRET[2],
                              zx_rapidhash_v3_read64(&p[24]) ^ s);
    }
  }

  static_assert(sizeof(i) == sizeof(zx_u64));
  a = zx_rapidhash_v3_read64(&p[i - 16]) ^ ((zx_u64)i);
  b = zx_rapidhash_v3_read64(&p[i - 8]);

  static_assert((sizeof(zx_u64) == sizeof(zx_isize)));
  return zx_rapidhash_v3_finish(a, b, s, (zx_u64)i);
}

static inline zx_u64 zx_rapidhash_v3(zx_u8 const *key, zx_u32 len,
                                     zx_u64 seed) {
  if (len <= 16) {
    return zx_rapidhash_v3_16(key, len, seed);
  } else {
    return zx_rapidhash_v3_long(key, len, seed);
  }
}

#endif

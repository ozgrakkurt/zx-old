// THIS CODE IS ADAPTED FROM
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/rapidhash.h
// Specifically the "Nano" version, which is optimized for code size and small
// inputs, is used here.
// Original code is licensed under MIT:
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/LICENSE
// Original License:
//
// Copyright 2025 Nicolas De Carli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZX_RAPIDHASH_V3_H
#define ZX_RAPIDHASH_V3_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

static constexpr zx_u64 ZX_RAPIDHASH_V3_SECRET_[8] = {
    0x2d358dccaa6c78a5, 0x8bb84b93962eacc9, 0x4b33a62ed433d4a3,
    0x4d5a2da51de1aa47, 0xa0761d6478bd642f, 0xe7037ed1a0b428db,
    0x90ed1765281c388c, 0xaaaaaaaaaaaaaaaa,
};

static inline void zx_rapidhash_v3_mum_(zx_u64* a, zx_u64* b) {
  zx_u128 const r = ((zx_u128){*a}) * ((zx_u128){*b});
  *a = (zx_u64)r;
  *b = (zx_u64)(r >> 64);
}

static inline zx_u64 zx_rapidhash_v3_mix_(zx_u64 a, zx_u64 b) {
  zx_rapidhash_v3_mum_(&a, &b);
  return a ^ b;
}

static inline zx_u64 zx_rapidhash_v3_finish_(zx_u64 a,
                                             zx_u64 b,
                                             zx_u64 seed,
                                             zx_u64 i) {
  a ^= ZX_RAPIDHASH_V3_SECRET_[1];
  b ^= seed;
  zx_rapidhash_v3_mum_(&a, &b);
  return zx_rapidhash_v3_mix_(a ^ ZX_RAPIDHASH_V3_SECRET_[7],
                              b ^ ZX_RAPIDHASH_V3_SECRET_[1] ^ i);
}

static inline zx_u64 zx_rapidhash_v3_16_(ZXSlice<zx_u8> key, zx_u64 seed) {
#ifdef ZX_DEV
  if (key.len() > 16) {
    __builtin_trap();
  }
#endif

  zx_u64 a = 0;
  zx_u64 b = 0;
  zx_u64 s = seed;

  s ^= zx_rapidhash_v3_mix_(s ^ ZX_RAPIDHASH_V3_SECRET_[2],
                            ZX_RAPIDHASH_V3_SECRET_[1]);

  if (key.len() >= 4) {
    s ^= key.len();
    if (key.len() >= 8) {
      a = key.read_unaligned_unsafe<zx_u64>(0);
      b = key.read_unaligned_unsafe<zx_u64>(zx_sub_unsafe_u64(key.len(), 8));
    } else {
      a = key.read_unaligned_unsafe<zx_u32>(0);
      b = key.read_unaligned_unsafe<zx_u32>(zx_sub_unsafe_u64(key.len(), 4));
    }
  } else if (key.len() > 0) {
    a = zx_shl_unsafe_u64((zx_u64)key.read_unsafe(0), 45) |
        ((zx_u64)key.read_unsafe(zx_sub_unsafe_u64(key.len(), 1)));
    b = (zx_u64)key.read_unsafe(zx_shr_unsafe_u64(key.len(), 1));
  } else {
    a = 0;
    b = 0;
  }

  return zx_rapidhash_v3_finish_(a, b, s, key.len());
}

static inline zx_u64 zx_rapidhash_v3_long_(ZXSlice<zx_u8> key, zx_u64 seed) {
#ifdef ZX_DEV
  if (key.len() <= 16) {
    __builtin_trap();
  }
#endif
  zx_u64 a = 0;
  zx_u64 b = 0;
  zx_u64 s = seed;

  zx_u64 offset = 0;

  s ^= zx_rapidhash_v3_mix_(s ^ ZX_RAPIDHASH_V3_SECRET_[2],
                            ZX_RAPIDHASH_V3_SECRET_[1]);

  if (key.len() > 48) {
    zx_u64 s0 = s;
    zx_u64 s1 = s;

    for (; zx_add_unsafe_u64(offset, 48) <= key.len();
         offset = zx_add_unsafe_u64(offset, 48)) {
      s = zx_rapidhash_v3_mix_(
          key.read_unaligned_unsafe<zx_u64>(offset) ^
              ZX_RAPIDHASH_V3_SECRET_[0],
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 8)) ^ s);

      s0 = zx_rapidhash_v3_mix_(
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 16)) ^
              ZX_RAPIDHASH_V3_SECRET_[1],
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 24)) ^
              s0);

      s1 = zx_rapidhash_v3_mix_(
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 32)) ^
              ZX_RAPIDHASH_V3_SECRET_[2],
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 40)) ^
              s1);
    }

    s ^= s0;
    s ^= s1;
  }

  if (zx_add_unsafe_u64(offset, 16) <= key.len()) {
    s = zx_rapidhash_v3_mix_(
        key.read_unaligned_unsafe<zx_u64>(offset) ^ ZX_RAPIDHASH_V3_SECRET_[2],
        key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 8)) ^ s);
    if (zx_add_unsafe_u64(offset, 32) <= key.len()) {
      s = zx_rapidhash_v3_mix_(
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 16)) ^
              ZX_RAPIDHASH_V3_SECRET_[2],
          key.read_unaligned_unsafe<zx_u64>(zx_add_unsafe_u64(offset, 24)) ^ s);
    }
  }

  zx_u64 const i = zx_sub_unsafe_u64(key.len(), offset);
  a = key.read_unaligned_unsafe<zx_u64>(zx_sub_unsafe_u64(key.len(), 16)) ^ i;
  b = key.read_unaligned_unsafe<zx_u64>(zx_sub_unsafe_u64(key.len(), 8));

  return zx_rapidhash_v3_finish_(a, b, s, i);
}

static inline zx_u64 zx_rapidhash_v3(ZXSlice<zx_u8> key, zx_u64 seed) {
  if (key.len() <= 16) {
    return zx_rapidhash_v3_16_(key, seed);
  } else {
    return zx_rapidhash_v3_long_(key, seed);
  }
}

#endif

// this file implements Xoshiro256++ algorithm for random number generation.
//
// Ported from code at:
// https://xoshiro.di.unimi.it/xoshiro256plusplus.c
//
// Original License:
// Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)
//
// To the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef ZX_XOSHIRO256_H
#define ZX_XOSHIRO256_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

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

#ifdef ZX_DEV
    {
      bool all_zero = true;
      for (zx_i8 i = 0; i < 4; i++) {
        all_zero &= rand.s[i] == 0;
      }
      if (all_zero) {
        __builtin_trap();
      }
    }
#endif

    return rand;
  }

  inline zx_u64 next() {
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

  inline void fill(ZXSliceMut<zx_u8> out) {
    if (out.len() == 0) {
      return;
    }

    zx_u64 const n_blocks = out.len() / sizeof(zx_u64);

    for (zx_u64 i = 0; i < n_blocks; i++) {
      zx_u64 const offset = zx_mul_unsafe_u64(i, sizeof(zx_u64));
      out.write_unaligned_unsafe<zx_u64>(offset, this->next());
    }

    zx_u64 const blocks_end = sizeof(zx_u64) * n_blocks;
    if (blocks_end < out.len()) {
      zx_u64 const cp_size = zx_sub_unsafe_u64(out.len(), blocks_end);
      zx_u64 const next = this->next();
      ZXSlice<zx_u8> const ws =
          ZXSlice<zx_u8>::init_unsafe((zx_u8 const*)&next, cp_size);
      ZXSliceMut<zx_u8> const os = out.slice_unsafe(blocks_end, cp_size);
      os.memcpy_from_unsafe(ws);
    }
  }
};

#endif

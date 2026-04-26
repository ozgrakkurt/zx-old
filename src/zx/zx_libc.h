// These functions are extracted to this header file for testing/benchmarking
// purposes. We can call these as zx_* so we know the compiler won't replace it
// with it's own inlined version of the function. If we use __builtin_* for
// testing/benchmarking then we risk compiler putting arbitrary code in there
// instead of calling these functions.
//
// These functions are then used in zx_libc.cc for actually implementing regular
// memcpy/memcmp etc. functions so linker can find those and compiler is able to
// conditionally replace __builtin_* calls with calls to these.

#ifndef ZX_LIBC_H
#define ZX_LIBC_H

#include <zx/zx_int.h>
#include <zx/zx_vec.h>

extern "C" {

__attribute__((no_builtin)) static inline void*
zx_memcpy(void* __restrict dest, void const* __restrict src, zx_u64_c count) {
  zx_u8 const* input = (zx_u8 const*)src;
  zx_u8* output = (zx_u8*)dest;

  zx_u64 offset = 0;

  for (; zx_add_unsafe_u64(offset, 64) < count;
       offset = zx_add_unsafe_u64(offset, 64)) {
    __asm__(
        "vmovdqu8 %[input], %%zmm31\n\t"
        "vmovdqu8 %%zmm31, %[output]\n\t"
        : [output] "=m"(*(zx_u8(*)[64]) & output[offset])
        : [input] "m"(*(zx_u8 const(*)[64]) & input[offset])
        : "zmm31");
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(count, offset);
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k7") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    __asm__(
        "vmovdqu8 %[input], %%zmm31 %{%[storemask]%}\n\t"
        "vmovdqu8 %%zmm31, %[output] %{%[storemask]%}\n\t"
        : [output] "=m"(*(zx_u8(*)[64]) & output[offset])
        : [input] "m"(*(zx_u8 const(*)[64]) & input[offset]), [storemask] "k"(
                                                                  storemask)
        : "zmm31");
  }

  return dest;
}

__attribute__((no_builtin)) static inline void* zx_memset(void* dest,
                                                          zx_i32_c ch,
                                                          zx_u64_c count) {
  zx_u8* output = (zx_u8*)dest;
  zx_u64_c const byte = (zx_u8)ch;

  zx_v512_u8 vbytes;
  __asm__(
      "vmovd %[byte], %%xmm31\n\t"
      "vpbroadcastb %%xmm31, %[vbytes]\n\t"
      : [vbytes] "=v"(vbytes)
      : [byte] "r"(byte)
      : "xmm31");

  zx_u64 offset = 0;

  for (; zx_add_unsafe_u64(offset, 64) < count;
       offset = zx_add_unsafe_u64(offset, 64)) {
    __asm__("vmovdqu8 %[vbytes], %[output]\n\t"
            : [output] "=m"(*(zx_u8(*)[64]) & output[offset])
            : [vbytes] "v"(vbytes));
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(count, offset);
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k2") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    __asm__("vmovdqu8 %[vbytes], %[output] %{%[storemask]%}\n\t"
            : [output] "=m"(*(zx_u8(*)[64]) & output[offset])
            : [vbytes] "v"(vbytes), [storemask] "k"(storemask));
  }

  return dest;
}

__attribute__((no_builtin)) static inline zx_i32_c zx_memcmp(void const* lhs,
                                                             void const* rhs,
                                                             zx_u64_c count) {
  zx_u8 const* left = (zx_u8 const*)lhs;
  zx_u8 const* right = (zx_u8 const*)rhs;

  zx_v512_u8 zeros;
  // TODO: is this the best way to do this?
  __asm__("vpxorq %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=v"(zeros));

  zx_u64 offset = 0;

  for (; zx_add_unsafe_u64(offset, 64) < count;
       offset = zx_add_unsafe_u64(offset, 64)) {
    zx_u64_c nonzero_mask;
    zx_v512_u8 diffs;
    __asm__(
        "vmovdqu8 %[left], %%zmm31\n\t"
        "vmovdqu8 %[right], %%zmm30\n\t"
        "vpsubb %%zmm30, %%zmm31, %[diffs]\n\t"
        // 4 means Not Equal OP
        "vpcmpub $4, %[diffs], %[zeros], %[nonzero_mask]\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask), [diffs] "=&v"(diffs)
        : [left] "m"(*(zx_u8 const(*)[64]) & left[offset]),
          [right] "m"(*(zx_u8 const(*)[64]) & right[offset]), [zeros] "v"(zeros)
        : "zmm30", "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
#ifdef ZX_DEV
      // idx can't be oob of the vector.
      if (idx >= 64) {
        __builtin_trap();
      }

      // it is supposed to be nonzero!!
      if (diffs[idx] == 0) {
        __builtin_trap();
      }
#endif
      return (zx_i8)diffs[idx];
    }
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(count, offset);
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k7") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    zx_u64_c nonzero_mask;
    zx_v512_u8 diffs;
    __asm__(
        "vmovdqu8 %[left], %%zmm31 %{%[storemask]%}\n\t"
        "vmovdqu8 %[right], %%zmm30 %{%[storemask]%}\n\t"
        "vpsubb %%zmm30, %%zmm31, %[diffs]\n\t"
        "kxorq %[nonzero_mask], %[nonzero_mask], %[nonzero_mask]\n\t"
        // 4 means Not Equal OP
        "vpcmpub $4, %[diffs], %[zeros], %[nonzero_mask] %{%[storemask]%}\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask), [diffs] "=&v"(diffs)
        : [left] "m"(*(zx_u8 const(*)[64]) & left[offset]),
          [right] "m"(*(zx_u8 const(*)[64]) & right[offset]),
          [zeros] "v"(zeros), [storemask] "k"(storemask)
        : "zmm30", "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
#ifdef ZX_DEV
      // idx can't be oob of the vector.
      if (idx >= 64) {
        __builtin_trap();
      }

      // idx can't be oob of remainder.
      if (idx >= remainder) {
        __builtin_trap();
      }

      // it is supposed to be nonzero!!
      if (diffs[idx] == 0) {
        __builtin_trap();
      }
#endif
      return (zx_i8)diffs[idx];
    }
  }

  return 0;
}

__attribute__((no_builtin)) static inline zx_u64_c zx_strlen(char const* str) {
  while (true) {
    // Do one page at a time so we don't accidentally read from the next page
    // which might not be mmapped. This should avoid segfaulting when processing
    // a string that is near the end of an allocated page.
    zx_u64 const count = zx_sub_unsafe_u64(
        zx_next_multiple_of_unsafe_u64(zx_add_unsafe_u64((zx_u64)str, 1), 4096),
        (zx_u64)str);

    zx_v512_u8 zeros;
    // TODO: is this the best way to do this?
    __asm__("vpxorq %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=v"(zeros));

    zx_u64 offset = 0;

    for (; zx_add_unsafe_u64(offset, 64) < count;
         offset = zx_add_unsafe_u64(offset, 64)) {
      zx_u64_c nonzero_mask;
      __asm__(
          "vmovdqu8 %[str], %%zmm31\n\t"
          // 4 means Not Equal OP
          "vpcmpub $4, %%zmm31, %[zeros], %[nonzero_mask]\n\t"
          : [nonzero_mask] "=&k"(nonzero_mask)
          : [str] "m"(*(char const(*)[64]) & str[offset]), [zeros] "v"(zeros)
          : "zmm31");
      if (nonzero_mask != 0) {
        // find first diff
        zx_u8 const idx = zx_ctz_u64(nonzero_mask);
#ifdef ZX_DEV
        // idx can't be oob of the vector.
        if (idx >= 64) {
          __builtin_trap();
        }
#endif
        return zx_add_unsafe_u64(offset, idx);
      }
    }

    zx_u64 const remainder = zx_sub_unsafe_u64(count, offset);
    if (remainder > 0) {
      register zx_u64_c const storemask __asm__("k7") =
          __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
      zx_u64_c nonzero_mask;
      __asm__(
          "vmovdqu8 %[str], %%zmm31 %{%[storemask]%}\n\t"
          "kxorq %[nonzero_mask], %[nonzero_mask], %[nonzero_mask]\n\t"
          // 4 means Not Equal OP
          "vpcmpub $4, %%zmm31, %[zeros], %[nonzero_mask] %{%[storemask]%}\n\t"
          : [nonzero_mask] "=&k"(nonzero_mask)
          : [str] "m"(*(char const(*)[64]) & str[offset]), [zeros] "v"(zeros),
            [storemask] "k"(storemask)
          : "zmm31");
      if (nonzero_mask != 0) {
        // find first diff
        zx_u8 const idx = zx_ctz_u64(nonzero_mask);
#ifdef ZX_DEV
        // idx can't be oob of the vector.
        if (idx >= 64) {
          __builtin_trap();
        }

        // idx can't be oob of remainder.
        if (idx >= remainder) {
          __builtin_trap();
        }
#endif
        return zx_add_unsafe_u64(offset, idx);
      }
    }
  }
}

}  // extern "C"

#endif

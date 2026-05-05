// This header isn't meant to be used except in testing contexts!!
//
// These functions are extracted to this header file for testing/benchmarking
// purposes. We name these as zx_* so we know the compiler won't replace it
// with it's own inlined version of the function. If we use __builtin_* for
// testing/benchmarking then we risk compiler putting inlined code in there
// instead of calling these functions.
//
// These functions are then used in zx_libc.cc for actually implementing regular
// memcpy/memcmp etc. functions so linker can find those if compiler inserts
// calls to these.

#ifndef ZX_LIBC_H
#define ZX_LIBC_H

#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_vec.h>

extern "C" {

__attribute__((no_builtin)) static inline void*
zx_memcpy(void* __restrict dest, void const* __restrict src, zx_u64_c count) {
  zx_u8 const* input = (zx_u8 const*)src;
  zx_u8* output = (zx_u8*)dest;

  zx_u64 offset = 0;

  for (; zx_add_unsafe_u64(offset, 256) <= count;
       offset = zx_add_unsafe_u64(offset, 256)) {
    __asm__(
        "vmovups (%[input]), %%zmm31\n\t"
        "vmovups 64(%[input]), %%zmm30\n\t"
        "vmovups 128(%[input]), %%zmm29\n\t"
        "vmovups 192(%[input]), %%zmm28\n\t"
        "vmovups %%zmm31, (%[output])\n\t"
        "vmovups %%zmm30, 64(%[output])\n\t"
        "vmovups %%zmm29, 128(%[output])\n\t"
        "vmovups %%zmm28, 192(%[output])\n\t"
        : "=m"(*(zx_u8(*)[256]) & output[offset])
        : "m"(*(zx_u8 const(*)[256]) & input[offset]),
          [input] "r"(&input[offset]), [output] "r"(&output[offset])
        : "zmm31", "zmm30", "zmm29", "zmm28");
  }

  for (; zx_add_unsafe_u64(offset, 64) <= count;
       offset = zx_add_unsafe_u64(offset, 64)) {
    __asm__(
        "vmovups (%[input]), %%zmm31\n\t"
        "vmovups %%zmm31, (%[output])\n\t"
        : "=m"(*(zx_u8(*)[64]) & output[offset])
        : "m"(*(zx_u8 const(*)[64]) & input[offset]),
          [input] "r"(&input[offset]), [output] "r"(&output[offset])
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

  for (; zx_add_unsafe_u64(offset, 64) <= count;
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

  zx_u64 offset = 0;
  for (; zx_add_unsafe_u64(offset, 64) <= count;
       offset = zx_add_unsafe_u64(offset, 64)) {
    zx_u64_c nonzero_mask;
    __asm__(
        "vmovdqu8 %[left], %%zmm31\n\t"
        "vmovdqu8 %[right], %%zmm30\n\t"
        // 4 means Not Equal OP
        "vpcmpub $4, %%zmm30, %%zmm31, %[nonzero_mask]\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask)
        : [left] "m"(*(zx_u8 const(*)[64]) & left[offset]),
          [right] "m"(*(zx_u8 const(*)[64]) & right[offset])
        : "zmm30", "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
      // idx can't be oob of the vector.
      zx_assert_dev(idx < 64);
      zx_u64 const pos = zx_add_unsafe_u64(offset, idx);
      zx_i32 const ret = zx_sub_unsafe_i32(left[pos], right[pos]);
      // can't be equal
      zx_assert_dev(ret != 0);
      return ret;
    }
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(count, offset);
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k7") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    zx_u64_c nonzero_mask;
    __asm__(
        "vmovdqu8 %[left], %%zmm31 %{%[storemask]%}%{z%}\n\t"
        "vmovdqu8 %[right], %%zmm30 %{%[storemask]%}%{z%}\n\t"
        // 4 means Not Equal OP
        "vpcmpub $4, %%zmm30, %%zmm31, %[nonzero_mask]\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask)
        : [left] "m"(*(zx_u8 const(*)[64]) & left[offset]),
          [right] "m"(*(zx_u8 const(*)[64]) & right[offset]),
          [storemask] "k"(storemask)
        : "zmm30", "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
      // idx can't be oob of the vector.
      zx_assert_dev(idx < 64);
      // idx can't be oob of remainder.
      zx_assert_dev(idx < remainder);
      zx_u64 const pos = zx_add_unsafe_u64(offset, idx);
      zx_i32 const ret = zx_sub_unsafe_i32(left[pos], right[pos]);
      // can't be equal
      zx_assert_dev(ret != 0);
      return ret;
    }
  }

  return 0;
}

__attribute__((no_builtin)) static inline zx_u64_c zx_strlen(char const* str) {
  zx_v512_u8 zeros;
  __asm__("vpxorq %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=v"(zeros));

  // align down if needed so we don't go over a page boundary (4096 > 64)
  if (!__builtin_is_aligned(str, 64)) {
    zx_u64 const addr = (zx_u64)str;
    zx_u64 const down_addr = __builtin_align_down(addr, 64);
    zx_u8 const diff =
        zx_cast_unsafe_to_u8<zx_u64>(zx_sub_unsafe_u64(addr, down_addr));
    register zx_u64_c const storemask __asm__("k7") =
        zx_shl_unsafe_u64(ZX_U64_MAX, diff);
    char const* down = (char const*)down_addr;
    zx_u64_c nonzero_mask;
    __asm__(
        "vmovdqu8 %[down], %%zmm31 %{%[storemask]%}\n\t"
        "kxorq %[nonzero_mask], %[nonzero_mask], %[nonzero_mask]\n\t"
        // 0 means Equal OP
        "vpcmpub $0, %%zmm31, %[zeros], %[nonzero_mask] %{%[storemask]%}\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask)
        : [down] "m"(*(char const(*)[64])down), [zeros] "v"(zeros),
          [storemask] "k"(storemask)
        : "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
      // idx can't be oob of the vector.
      zx_assert_dev(idx < 64);
      return zx_sub_unsafe_u8(idx, diff);
    }
  }

  // read 64 byte aligned chunks so we never go over a page boundary (4096 > 64)
  zx_u64 offset =
      zx_sub_unsafe_u64(__builtin_align_up((zx_u64)str, 64), (zx_u64)str);

  for (;; offset = zx_add_unsafe_u64(offset, 64)) {
    zx_u64_c nonzero_mask;
    __asm__(
        "vmovdqu8 %[str], %%zmm31\n\t"
        // 0 means Equal OP
        "vpcmpub $0, %%zmm31, %[zeros], %[nonzero_mask]\n\t"
        : [nonzero_mask] "=&k"(nonzero_mask)
        : [str] "m"(*(char const(*)[64]) & str[offset]), [zeros] "v"(zeros)
        : "zmm31");
    if (nonzero_mask != 0) {
      // find first diff
      zx_u8 const idx = zx_ctz_u64(nonzero_mask);
      // idx can't be oob of the vector.
      zx_assert_dev(idx < 64);
      return zx_add_unsafe_u64(offset, idx);
    }
  }
}

}  // extern "C"

#endif

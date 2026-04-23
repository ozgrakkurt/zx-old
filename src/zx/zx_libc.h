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

  zx_u64 const remainder = count - offset;
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k2") =
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

  zx_u64 const remainder = count - offset;
  if (remainder > 0) {
    register zx_u64_c const storemask __asm__("k2") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    __asm__("vmovdqu8 %[vbytes], %[output] %{%[storemask]%}\n\t"
            : [output] "=m"(*(zx_u8(*)[64]) & output[offset])
            : [vbytes] "v"(vbytes), [storemask] "k"(storemask));
  }

  return dest;
}

}  // extern "C"

#endif

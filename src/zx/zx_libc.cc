// This file defines some libc functions that are used by the compiler builtins.
// We don't link to a real libc or use any libc functions in the code. But we
// still have to define these since we use functions like __builtin_memcpy and
// also the compiler sometimes inserts calls to these functions implicitly as an
// optimization.

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_int.h>
#include <zx/zx_vec.h>

// Define _start function that calls the user defined main.
__asm__(
    ".type _start, @function\n"
    ".global _start\n"
    "_start:\n\t"
    "xorq %rbp, %rbp\n\t"
    "popq %rdi\n\t"
    "movq %rsp, %rsi\n\t"
    "andq $-16, %rsp\n\t"
    "callq main\n\t"
    "movq %rax, %rdi\n\t"
    "movq $60, %rax\n\t"
    "syscall\n\t");

extern "C" {

__attribute__((no_builtin)) void* memcpy(void* __restrict dest,
                                         void const* __restrict src,
                                         zx_u64_c count) {
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

__attribute__((no_builtin)) void* memset(void* dest,
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

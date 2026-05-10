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

#include "./zx_libc.h"
#include <zx/zx_int.h>

// Define _start function that calls the user defined main.
__asm__(".type _start, @function\n"
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

__attribute__((no_builtin)) void *
memcpy(void *__restrict dest, void const *__restrict src, zx_u64_c count) {
  return zx_memcpy(dest, src, count);
}

__attribute__((no_builtin)) void *memset(void *dest, zx_i32_c ch,
                                         zx_u64_c count) {
  return zx_memset(dest, ch, count);
}

__attribute__((no_builtin)) zx_i32_c memcmp(void const *lhs, void const *rhs,
                                            zx_u64_c count) {
  return zx_memcmp(lhs, rhs, count);
}

__attribute__((no_builtin)) zx_u64_c strlen(char const *str) {
  return zx_strlen(str);
}

} // extern "C"

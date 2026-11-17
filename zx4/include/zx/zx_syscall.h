#ifndef ZX_SYSCALL_H
#define ZX_SYSCALL_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_int.h>

static inline zx_u64_c zx_syscall0(zx_u64_c num) {
  zx_u64_c ret;

  __asm__ volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall1(zx_u64_c num, zx_u64_c arg1) {
  zx_u64_c ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1)
                   : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall2(zx_u64_c num, zx_u64_c arg1, zx_u64_c arg2) {
  zx_u64_c ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2)
                   : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall3(zx_u64_c num, zx_u64_c arg1, zx_u64_c arg2,
                                   zx_u64_c arg3) {
  zx_u64_c ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
                   : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall4(zx_u64_c num, zx_u64_c arg1, zx_u64_c arg2,
                                   zx_u64_c arg3, zx_u64_c arg4) {
  zx_u64_c ret;
  register zx_u64_c r10 __asm__("r10") = arg4;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                   : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall5(zx_u64_c num, zx_u64_c arg1, zx_u64_c arg2,
                                   zx_u64_c arg3, zx_u64_c arg4,
                                   zx_u64_c arg5) {
  zx_u64_c ret;
  register zx_u64_c r10 __asm__("r10") = arg4;
  register zx_u64_c r8 __asm__("r8") = arg5;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                     "r"(r8)
                   : "rcx", "r11", "memory");

  return ret;
}

static inline zx_u64_c zx_syscall6(zx_u64_c num, zx_u64_c arg1, zx_u64_c arg2,
                                   zx_u64_c arg3, zx_u64_c arg4, zx_u64_c arg5,
                                   zx_u64_c arg6) {
  zx_u64_c ret;
  register zx_u64_c r10 __asm__("r10") = arg4;
  register zx_u64_c r8 __asm__("r8") = arg5;
  register zx_u64_c r9 __asm__("r9") = arg6;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                     "r"(r8), "r"(r9)
                   : "rcx", "r11", "memory");

  return ret;
}

#endif

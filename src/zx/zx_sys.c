#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_def.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_status.h>

typedef unsigned long zx_sys_reg;
static_assert(sizeof(zx_sys_reg) == sizeof(zx_sys_reg));
static_assert(alignof(zx_sys_reg) == alignof(zx_sys_reg));

[[maybe_unused]]
static zx_sys_reg zx_syscall0(zx_sys_reg num) {
  zx_sys_reg ret;

  __asm__ volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall1(zx_sys_reg num, zx_sys_reg arg1) {
  zx_sys_reg ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1)
                   : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall2(zx_sys_reg num, zx_sys_reg arg1,
                              zx_sys_reg arg2) {
  zx_sys_reg ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2)
                   : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall3(zx_sys_reg num, zx_sys_reg arg1, zx_sys_reg arg2,
                              zx_sys_reg arg3) {
  zx_sys_reg ret;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
                   : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall4(zx_sys_reg num, zx_sys_reg arg1, zx_sys_reg arg2,
                              zx_sys_reg arg3, zx_sys_reg arg4) {
  zx_sys_reg ret;
  register zx_sys_reg r10 __asm__("r10") = arg4;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                   : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall5(zx_sys_reg num, zx_sys_reg arg1, zx_sys_reg arg2,
                              zx_sys_reg arg3, zx_sys_reg arg4,
                              zx_sys_reg arg5) {
  zx_sys_reg ret;
  register zx_sys_reg r10 __asm__("r10") = arg4;
  register zx_sys_reg r8 __asm__("r8") = arg5;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                     "r"(r8)
                   : "rcx", "r11", "memory");

  return ret;
}

[[maybe_unused]]
static zx_sys_reg zx_syscall6(zx_sys_reg num, zx_sys_reg arg1, zx_sys_reg arg2,
                              zx_sys_reg arg3, zx_sys_reg arg4, zx_sys_reg arg5,
                              zx_sys_reg arg6) {
  zx_sys_reg ret;
  register zx_sys_reg r10 __asm__("r10") = arg4;
  register zx_sys_reg r8 __asm__("r8") = arg5;
  register zx_sys_reg r9 __asm__("r9") = arg6;

  __asm__ volatile("syscall"
                   : "=a"(ret)
                   : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                     "r"(r8), "r"(r9)
                   : "rcx", "r11", "memory");

  return ret;
}

static zx_usize zx_sys_errno(zx_sys_reg ret) {
  static_assert(sizeof(zx_sys_reg) == sizeof(zx_i64));
  zx_i64 const sgnd = (zx_i64)ret;
  if (sgnd > -4096 && sgnd < 0) {
    return (zx_u64)-sgnd;
  } else {
    return ZX_SYS_STATUS_SUCCESS;
  }
}

constexpr zx_sys_reg ZX_SYSCALL_READ = 0;
constexpr zx_sys_reg ZX_SYSCALL_WRITE = 1;
constexpr zx_sys_reg ZX_SYSCALL_OPEN = 2;
constexpr zx_sys_reg ZX_SYSCALL_CLOSE = 3;
constexpr zx_sys_reg ZX_SYSCALL_MMAP = 9;
constexpr zx_sys_reg ZX_SYSCALL_MUNMAP = 11;
constexpr zx_sys_reg ZX_SYSCALL_MADVISE = 28;
constexpr zx_sys_reg ZX_SYSCALL_EXIT_GROUP = 231;

zx_usize zx_sys_read(zx_i32 fd, zx_u8 *buf, zx_usize size, zx_usize *n_read) {
  constexpr zx_usize MAX = 0x7ffff000wbu;
  zx_usize const read_len = (MAX <= size) ? MAX : size;

  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_READ, (zx_sys_reg)fd,
                                     (zx_sys_reg)buf, (zx_sys_reg)read_len);
  zx_usize const status = zx_sys_errno(ret);

  if (status == ZX_SYS_STATUS_SUCCESS) {
    *n_read = (zx_usize){ret};
  } else {
    *n_read = 0;
  }

  return status;
}

zx_usize zx_sys_write(zx_i32 fd, zx_u8 const *buf, zx_usize size,
                      zx_usize *n_written) {
  constexpr zx_usize MAX = 0x7ffff000wbu;
  zx_usize const write_len = (MAX <= size) ? MAX : size;

  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_WRITE, (zx_sys_reg)fd,
                                     (zx_sys_reg)buf, (zx_sys_reg)write_len);
  zx_usize const status = zx_sys_errno(ret);

  if (status == ZX_SYS_STATUS_SUCCESS) {
    *n_written = (zx_usize){ret};
  } else {
    *n_written = 0;
  }

  return status;
}

[[noreturn]]
void zx_sys_abort(void) {
  ZXSysStatus const status =
      zx_sys_errno(zx_syscall1(ZX_SYSCALL_EXIT_GROUP, (zx_sys_reg){127}));
  (void)status;
  __builtin_unreachable();
}

zx_usize zx_sys_mmap(void *address, zx_usize length, zx_i32 prot, zx_i32 flags,
                     zx_i32 fd, zx_i64 offset, void **mem) {
  zx_sys_reg const ret = zx_syscall6(
      ZX_SYSCALL_MMAP, (zx_sys_reg)address, (zx_sys_reg)length,
      (zx_sys_reg)prot, (zx_sys_reg)flags, (zx_sys_reg)fd, (zx_sys_reg)offset);
  zx_usize const status = zx_sys_errno(ret);

  if (status == ZX_SYS_STATUS_SUCCESS) {
    *mem = (void *)ret;
  } else {
    *mem = nullptr;
  }

  return status;
}

zx_usize zx_sys_munmap(void *mem, zx_usize length) {
  return zx_sys_errno(
      zx_syscall2(ZX_SYSCALL_MUNMAP, (zx_sys_reg)mem, (zx_sys_reg)length));
}

zx_usize zx_sys_madvise(void *mem, zx_usize length, zx_i32 advice) {
  return zx_sys_errno(zx_syscall3(ZX_SYSCALL_MADVISE, (zx_sys_reg)mem,
                                  (zx_sys_reg)length, (zx_sys_reg)advice));
}

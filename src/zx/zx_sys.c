#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

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

constexpr zx_u64 ZX_SYS_STATUS_SUCCESS = 0;

static zx_u64 zx_sys_errno(zx_sys_reg ret) {
  static_assert(sizeof(zx_sys_reg) == sizeof(zx_i64));
  zx_i64 const sgnd = (zx_i64)ret;
  if (sgnd > -4096 && sgnd < 0) {
    return (zx_u64)-sgnd;
  } else {
    return ZX_SYS_STATUS_SUCCESS;
  }
}

static ZXStatus zx_sys_status(zx_sys_reg ret) {
  switch (zx_sys_errno(ret)) {
  case ZX_SYS_STATUS_SUCCESS: {
    return ZXStatus_Ok;
  }
  default: {
    return ZXStatus_SysErrUnexpected;
  }
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

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "io_uring"
constexpr zx_sys_reg ZX_SYSCALL_IO_URING_SETUP = 425;
constexpr zx_sys_reg ZX_SYSCALL_IO_URING_ENTER = 426;
constexpr zx_sys_reg ZX_SYSCALL_IO_URING_REGISTER = 427;

ZXStatus zx_sys_read(zx_i32 fd, zx_u8 *buf, zx_u64 size, zx_u64 *n_read,
                     ZXErrStack *err_stack) {
  constexpr zx_u64 MAX = 0x7ffff000wbu;
  zx_u64 const read_len = (MAX <= size) ? MAX : size;

  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_READ, (zx_sys_reg)fd,
                                     (zx_sys_reg)buf, (zx_sys_reg)read_len);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }
  *n_read = (zx_u64){ret};

  return ZXStatus_Ok;
}

ZXStatus zx_sys_write(zx_i32 fd, zx_u8 const *buf, zx_u64 size,
                      zx_u64 *n_written, ZXErrStack *err_stack) {
  constexpr zx_u64 MAX = 0x7ffff000wbu;
  zx_u64 const write_len = (MAX <= size) ? MAX : size;

  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_WRITE, (zx_sys_reg)fd,
                                     (zx_sys_reg)buf, (zx_sys_reg)write_len);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }
  *n_written = (zx_u64){ret};

  return ZXStatus_Ok;
}

ZXStatus zx_sys_open(char const *pathname, zx_i32 flags, zx_u32 mode,
                     zx_i32 *fd, ZXErrStack *err_stack) {
  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_OPEN, (zx_sys_reg)pathname,
                                     (zx_sys_reg)flags, (zx_sys_reg)mode);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }
  *fd = (zx_i32)ret;

  return ZXStatus_Ok;
}

ZXStatus zx_sys_close(zx_i32 fd, ZXErrStack *err_stack) {
  zx_sys_reg const ret = zx_syscall1(ZX_SYSCALL_CLOSE, (zx_sys_reg)fd);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  return ZXStatus_Ok;
}

[[noreturn]]
void zx_sys_abort(void) {
  zx_u64 const status =
      zx_sys_errno(zx_syscall1(ZX_SYSCALL_EXIT_GROUP, (zx_sys_reg){127}));
  (void)status;
  __builtin_unreachable();
}

[[noreturn]]
void zx_sys_exit(int code) {
  zx_u64 const status =
      zx_sys_errno(zx_syscall1(ZX_SYSCALL_EXIT_GROUP, (zx_sys_reg)code));
  (void)status;
  __builtin_unreachable();
}

ZXStatus zx_sys_mmap(zx_u8 *address, zx_u64 length, zx_i32 prot, zx_i32 flags,
                     zx_i32 fd, zx_i64 offset, zx_u8 **mem,
                     ZXErrStack *err_stack) {
  zx_sys_reg const ret = zx_syscall6(
      ZX_SYSCALL_MMAP, (zx_sys_reg)address, (zx_sys_reg)length,
      (zx_sys_reg)prot, (zx_sys_reg)flags, (zx_sys_reg)fd, (zx_sys_reg)offset);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }
  *mem = (zx_u8 *)ret;

  return ZXStatus_Ok;
}

ZXStatus zx_sys_munmap(zx_u8 *mem, zx_u64 length, ZXErrStack *err_stack) {
  zx_sys_reg const ret =
      zx_syscall2(ZX_SYSCALL_MUNMAP, (zx_sys_reg)mem, (zx_sys_reg)length);
  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  return ZXStatus_Ok;
}

ZXStatus zx_sys_madvise(zx_u8 *mem, zx_u64 length, zx_i32 advice,
                        ZXErrStack *err_stack) {
  zx_sys_reg const ret = zx_syscall3(ZX_SYSCALL_MADVISE, (zx_sys_reg)mem,
                                     (zx_sys_reg)length, (zx_sys_reg)advice);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  return ZXStatus_Ok;
}

ZXStatus zx_sys_io_uring_setup(zx_u32 n_entries, struct io_uring_params *params,
                               zx_i32 *fd, ZXErrStack *err_stack) {
  zx_sys_reg const ret = zx_syscall2(ZX_SYSCALL_IO_URING_SETUP,
                                     (zx_sys_reg)n_entries, (zx_sys_reg)params);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  fd = (zx_i32)ret;

  return ZXStatus_Ok;
}

ZXStatus zx_sys_io_uring_enter(zx_i32 fd, zx_u32 to_submit, zx_u32 min_complete,
                               zx_u32 flags, void *arg, zx_u64 sz,
                               ZXErrStack *err_stack) {
  zx_sys_reg const ret =
      zx_syscall6(ZX_SYSCALL_IO_URING_ENTER, (zx_sys_reg)fd,
                  (zx_sys_reg)to_submit, (zx_sys_reg)min_complete,
                  (zx_sys_reg)flags, (zx_sys_reg)arg, (zx_sys_reg)sz);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  return ZXStatus_Ok;
}

ZXStatus zx_sys_io_uring_register(zx_i32 fd, zx_u32 opcode, void const *arg,
                                  zx_u32 nr_args, ZXErrStack *err_stack) {
  zx_sys_reg const ret =
      zx_syscall4(ZX_SYSCALL_IO_URING_REGISTER, (zx_sys_reg)fd,
                  (zx_sys_reg)opcode, (zx_sys_reg)arg, (zx_sys_reg)nr_args);

  ZXStatus const status = zx_sys_status(ret);
  if (status != ZXStatus_Ok) {
    ZX_ERR_RETURN(status, err_stack);
  }

  return ZXStatus_Ok;
}

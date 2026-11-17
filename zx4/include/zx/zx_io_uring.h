#ifndef ZX_IO_URING_H
#define ZX_IO_URING_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_int.h>
#include <zx/zx_io_uring_c.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_syscall.h>

static inline ZXSysStatus
zx_sys_io_uring_setup(zx_u32 n_entries, io_uring_params *params, zx_i32 *fd) {
  if (fd == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const ret =
      zx_syscall2(ZX_SYSCALL_IO_URING_SETUP, n_entries, (zx_u64)params);

  ZXSysStatus const status = zx_sys_status_(ret);
  if (status != ZXSysStatus::Ok) {
    *fd = -1;
    return status;
  }

  zx_i32 const ret_fd = (zx_i32)ret;
  if (ret_fd <= 0) {
    *fd = -1;
    return ZXSysStatus::UnexpectedError;
  }

  *fd = ret_fd;
  return ZXSysStatus::Ok;
}

static inline ZXSysStatus zx_sys_io_uring_enter(zx_i32 fd, zx_u32 to_submit,
                                                zx_u32 min_complete,
                                                zx_u32 flags, void *arg,
                                                zx_u64 sz) {
  zx_u64 const ret =
      zx_syscall6(ZX_SYSCALL_IO_URING_ENTER, (zx_u64)fd, to_submit,
                  min_complete, flags, (zx_u64)arg, sz);
  return zx_sys_status_(ret);
}

static inline ZXSysStatus zx_sys_io_uring_register(zx_i32 fd, zx_u32 opcode,
                                                   void const *arg,
                                                   zx_u32 nr_args) {
  zx_u64 const ret = zx_syscall4(ZX_SYSCALL_IO_URING_REGISTER, (zx_u64)fd,
                                 opcode, (zx_u64)arg, nr_args);
  return zx_sys_status_(ret);
}

#endif

#ifndef ZX_PERF_EVENT_H
#define ZX_PERF_EVENT_H

#include <zx/zx_int.h>
#include <zx/zx_perf_event_c.h>
#include <zx/zx_sys.h>
#include <zx/zx_syscall.h>

static inline ZXSysStatus zx_sys_perf_event_open(perf_event_attr *attr,
                                                 zx_i32 pid, zx_i32 cpu,
                                                 zx_i32 group_fd, zx_u64 flags,
                                                 zx_i32 *fd) {
  if (fd == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const ret =
      zx_syscall5(ZX_SYSCALL_PERF_EVENT_OPEN, (zx_u64)attr, (zx_u64)pid,
                  (zx_u64)cpu, (zx_u64)group_fd, flags);

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

#endif

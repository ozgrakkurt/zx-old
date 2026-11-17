#ifndef ZX_STATX_H
#define ZX_STATX_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_statx_c.h>
#include <zx/zx_sys.h>

static inline ZXSysStatus zx_sys_statx(zx_i32 dirfd, ZXCString path,
                                       zx_i32 flags, zx_u32 mask,
                                       statx *statxbuf) {
  if (statxbuf == nullptr) {
    return ZXSysStatus::Nullptr;
  }
  __builtin_memset(statxbuf, 0, sizeof(statx));
  zx_u64 const ret =
      zx_syscall5(ZX_SYSCALL_STATX, (zx_u64)dirfd, (zx_u64)path.as_cstr(),
                  (zx_u64)flags, (zx_u64)mask, (zx_u64)statxbuf);
  return zx_sys_status_(ret);
}

#endif

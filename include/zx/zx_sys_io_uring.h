#ifndef ZX_SYS_IO_URING_H
#define ZX_SYS_IO_URING_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct ZXSysIoUringCQOffsets {
  zx_u32 head;
  zx_u32 tail;
  zx_u32 ring_mask;
  zx_u32 ring_entries;
  zx_u32 overflow;
  zx_u32 cqes;
  zx_u32 flags;
  zx_u32 resv1;
  zx_u64 user_addr;
} ZXSysIoUringCQOffsets;

typedef struct ZXSysIoUringSQOffsets {
  zx_u32 head;
  zx_u32 tail;
  zx_u32 ring_mask;
  zx_u32 ring_entries;
  zx_u32 flags;
  zx_u32 dropped;
  zx_u32 array;
  zx_u32 resv1;
  zx_u64 user_addr;
} ZXSysIoUringSQOffsets;

typedef struct ZXSysIoUringParams {
  zx_u32 sq_entries;
  zx_u32 cq_entries;
  zx_u32 flags;
  zx_u32 sq_thread_cpu;
  zx_u32 sq_thread_idle;
  zx_u32 features;
  zx_u32 wq_fd;
  zx_u32 resv[3];
  ZXSysIoUringSQOffsets sq_off;
  ZXSysIoUringCQOffsets cq_off;
} ZXSysIoUringParams;

ZXStatus zx_sys_io_uring_setup(zx_u32 entries, ZXSysIoUringParams *params,
                               ZXErrStack *err_stack);

ZXStatus zx_sys_io_uring_enter(zx_i32 fd, zx_u32 to_submit, zx_u32 min_complete,
                               zx_u32 flags, void *arg, zx_u64 sz,
                               ZXErrStack *err_stack);

ZXStatus zx_sys_io_uring_register(zx_i32 fd, zx_u32 opcode, void const *arg,
                                  zx_u32 nr_args, ZXErrStack *err_stack);

#endif

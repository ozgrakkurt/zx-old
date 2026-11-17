#ifndef ZX_SYS_IO_URING_H
#define ZX_SYS_IO_URING_H

#include <zx/io_uring.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

ZXStatus zx_sys_io_uring_setup(zx_u32 n_entries, struct io_uring_params *params,
                               zx_i32 *fd, ZXErrStack *err_stack);

ZXStatus zx_sys_io_uring_enter(zx_i32 fd, zx_u32 to_submit, zx_u32 min_complete,
                               zx_u32 flags, void *arg, zx_u64 sz,
                               ZXErrStack *err_stack);

ZXStatus zx_sys_io_uring_register(zx_i32 fd, zx_u32 opcode, void const *arg,
                                  zx_u32 nr_args, ZXErrStack *err_stack);

#endif

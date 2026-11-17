#ifndef ZX_IO_H
#define ZX_IO_H

#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

static inline ZXStatus zx_io_print(zx_i32 fd, ZXFmt const *fmt,
                                   ZXErrStack *err_stack) {
  zx_u64 const to_write = ZXFmt_n_written(fmt);
  zx_u64 n_written = 0;
  ZX_ERR_TRY((zx_sys_write(fd, (zx_u8 const *)ZXFmt_buf(fmt), to_write,
                           &n_written, err_stack)),
             err_stack);

  if (n_written < to_write) {
    return ZXStatus_ShortWrite;
  } else if (n_written > to_write) {
    return ZXStatus_LongWrite;
  } else {
    return ZXStatus_Ok;
  }
}

#endif

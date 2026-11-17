#ifndef STREAMVBYTE_H
#define STREAMVBYTE_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

static inline ZXStatus streamvbyte_compress_bound(zx_u32 input_len,
                                                  zx_u32 *n_bytes,
                                                  ZXErrStack *err_stack) {
  constexpr zx_u8 PADDING = 16;

  zx_u64 const header_size = ((zx_u64){input_len} + 3) / 4;
  zx_u64 const data_size = ((zx_u64){input_len}) * sizeof(zx_u32);
  zx_u64 const total = header_size + data_size + PADDING;
  ZX_ERR_ASSERT((total < ZX_U32_MAX), err_stack);
  *n_bytes = (zx_u32)total;
  return ZXStatus_Ok;
}

ZXStatus streamvbyte_0124_encode(zx_u32 const *restrict input, zx_u32 input_len,
                                 zx_u8 *restrict output, zx_u32 output_size,
                                 zx_u32 *n_bytes_written,
                                 ZXErrStack *err_stack);
ZXStatus streamvbyte_0124_decode(zx_u8 const *restrict input, zx_u32 input_size,
                                 zx_u32 *restrict output, zx_u32 output_len,
                                 zx_u32 *n_bytes_read, ZXErrStack *err_stack);

#endif

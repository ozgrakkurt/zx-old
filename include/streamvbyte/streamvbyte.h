#ifndef STREAMVBYTE_H
#define STREAMVBYTE_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>

ShinStatus streamvbyte_compress_bound(shin_u32 input_len, shin_u32 *n_bytes,
                                      ShinErrStack *err_stack) {
  constexpr shin_u8 PADDING = 16;

  shin_u64 const header_size = ((shin_u64){input_len} + 3) / 4;
  shin_u64 const data_size = ((shin_u64){input_len}) * sizeof(shin_u32);
  shin_u64 const total = header_size + data_size + PADDING;
  SHIN_ERR_ASSERT((total < SHIN_UINT_MAX(shin_u32)), err_stack);
  *n_bytes = (shin_u32)total;
  return ShinStatus_Ok;
}

ShinStatus streamvbyte_0124_encode(shin_u32 const *restrict input,
                                   shin_u32 input_len, shin_u8 *restrict output,
                                   shin_u32 output_size,
                                   shin_u32 *n_bytes_written,
                                   ShinErrStack *err_stack);
ShinStatus streamvbyte_0124_decode(shin_u8 const *restrict input,
                                   shin_u32 input_size,
                                   shin_u32 *restrict output,
                                   shin_u32 output_len, shin_u32 *n_bytes_read,
                                   ShinErrStack *err_stack);

#endif

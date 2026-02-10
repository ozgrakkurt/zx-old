#ifndef STREAMVBYTE_H
#define STREAMVBYTE_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>

#define STREAMVBYTE_PADDING 16

ShinStatus streamvbyte_compress_bound(shin_u32 input_len, shin_u32 *n_bytes,
                                      ShinErrStack *err_stack);
ShinStatus streamvbyte_encode_0124(shin_u32 const *restrict input,
                                   shin_u32 input_len, shin_u8 *restrict output,
                                   shin_u32 output_size,
                                   shin_u32 *n_bytes_written,
                                   ShinErrStack *err_stack);
ShinStatus streamvbyte_decode_0124(shin_u8 const *restrict input,
                                   shin_u32 input_size,
                                   shin_u32 *restrict output,
                                   shin_u32 output_len, shin_u32 *n_bytes_read,
                                   ShinErrStack *err_stack);

#endif

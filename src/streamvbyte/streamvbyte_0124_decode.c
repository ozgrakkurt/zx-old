#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE4_1__
#error "sse4.1 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

#include <immintrin.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <streamvbyte/streamvbyte.h>
#include <string.h>

#include "./streamvbyte_shuffle_tables_0124_decode.h"

static inline ShinStatus
streamvbyte_decode_0124_validate(shin_u8 const *restrict input,
                                 shin_u32 input_size, shin_u32 output_len,
                                 ShinErrStack *err_stack) {
  if (input_size == 0) {
    SHIN_ERR_ASSERT((output_len == 0), err_stack);
    return ShinStatus_Ok;
  }
  SHIN_ERR_ASSERT((output_len > 0), err_stack);

  shin_u64 const keys_size = ((shin_u64){output_len} + 3) / 4;
  SHIN_ERR_ASSERT((keys_size <= input_size), err_stack);

  shin_u64 encoded_size = 0wbu;
  shin_u32 const whole_len = output_len / 4;
  for (shin_u32 i = 0; i < whole_len; i++) {
    shin_u8 const key = input[i];
    for (shin_u8 shift = 0; shift < 8; shift += 2) {
      shin_u8 const code = (key >> shift) & 0x3wbu;
      encoded_size += ((shin_u32){1wbu} << code) >> 1;
    }
  }

  shin_u32 const remainder_len = output_len % 4;
  if (remainder_len > 0) {
    shin_u8 const key = input[whole_len];
    shin_u8 shift = 0;
    for (shin_u32 i = 0; i < remainder_len; i++) {
      SHIN_ASSERT((shift < 8));
      shin_u8 const code = (key >> shift) & 0x3wbu;
      encoded_size += ((shin_u32){1wbu} << code) >> 1;
      shift += 2;
    }
  }

  SHIN_ERR_ASSERT((encoded_size + keys_size <= input_size), err_stack);

  return ShinStatus_Ok;
}

ShinStatus streamvbyte_decode_0124(shin_u8 const *restrict input,
                                   shin_u32 input_size,
                                   shin_u32 *restrict output,
                                   shin_u32 output_len, shin_u32 *n_bytes_read,
                                   ShinErrStack *err_stack) {
  SHIN_ERR_TRY((streamvbyte_decode_0124_validate(input, input_size, output_len,
                                                 err_stack)),
               err_stack);

  if (output_len == 0) {
    return ShinStatus_Ok;
  }

  (void)output;
  (void)n_bytes_read;

  return ShinStatus_Ok;
}

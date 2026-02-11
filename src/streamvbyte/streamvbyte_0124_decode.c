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

static inline shin_u8 streamvbyte_0124_decode_code_to_input_size(shin_u8 code) {
  switch (code) {
  case 0: {
    return 0;
  }
  case 1: {
    return 1;
  }
  case 2: {
    return 2;
  }
  case 3: {
    return 4;
  }
  default: {
    SHIN_DBG_ASSERT((false));
    return 0;
  }
  }
}

static inline ShinStatus
streamvbyte_0124_decode_validate(shin_u8 const *restrict input,
                                 shin_u32 input_size, shin_u32 output_len,
                                 ShinErrStack *err_stack) {
  if (output_len == 0) {
    return ShinStatus_Ok;
  }
  SHIN_ERR_ASSERT((output_len > 0), err_stack);

  // 2 bits per key, 8bits (one byte) per 4 keys.
  // rounded up for remainder.
  shin_u64 const header_size = ((shin_u64){output_len} + 3wbu) / 4wbu;
  SHIN_ERR_ASSERT((header_size <= input_size), err_stack);

  shin_u64 data_size = 0wbu;
  shin_u32 const whole_len = output_len / 4wbu;
  for (shin_u32 i = 0; i < whole_len; i++) {
    shin_u8 const key = input[i];
    for (shin_u8 shift = 0; shift < 8; shift += 2) {
      shin_u8 const code = (key >> shift) & 0x3wbu;
      data_size += streamvbyte_0124_decode_code_to_input_size(code);
    }
  }

  shin_u32 const remainder_len = output_len % 4wbu;
  if (remainder_len > 0) {
    shin_u8 const key = input[whole_len];
    shin_u8 shift = 0;
    for (shin_u32 i = 0; i < remainder_len; i++) {
      SHIN_ASSERT((shift < 8));
      shin_u8 const code = (key >> shift) & 0x3wbu;
      data_size += streamvbyte_0124_decode_code_to_input_size(code);
      shift += 2;
    }
  }

  constexpr shin_u8 PADDING = 16;
  SHIN_ERR_ASSERT((data_size + header_size + PADDING <= input_size), err_stack);

  return ShinStatus_Ok;
}

ShinStatus streamvbyte_0124_decode(shin_u8 const *restrict input,
                                   shin_u32 input_size,
                                   shin_u32 *restrict output,
                                   shin_u32 output_len, shin_u32 *n_bytes_read,
                                   ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((output != nullptr));
  SHIN_ASSERT((n_bytes_read != nullptr));

  SHIN_ERR_TRY((streamvbyte_0124_decode_validate(input, input_size, output_len,
                                                 err_stack)),
               err_stack);

  if (output_len == 0) {
    *n_bytes_read = 0;
    return ShinStatus_Ok;
  }

  shin_u32 const header_size =
      (shin_u32)(((shin_u64){output_len} + 3wbu) / 4wbu);

  shin_u32 const n_whole = output_len / 4wbu;
  shin_u32 const n_remainder = output_len % 4wbu;

  shin_u32 in_offset = 0;

  for (shin_u32 i = 0; i < n_whole; i++) {
    shin_u8 const key = input[i];
    __m128i const in =
        _mm_loadu_si128((__m128i const *)&input[header_size + in_offset]);
    __m128i const shuffle = _mm_loadu_si128(
        (__m128i const
             *)&STREAMVBYTE_SHUFFLE_TABLES_0124_DECODE_SHUFFLE_TABLE[key]);
    __m128i const out = _mm_shuffle_epi8(in, shuffle);
    _mm_storeu_si128((__m128i *)&output[i * 4], out);
    in_offset += STREAMVBYTE_SHUFFLE_TABLES_0124_DECODE_LENGTH_TABLE[key];
  }

  if (n_remainder > 0) {
    shin_u32 const out_offset = n_whole * 4;
    shin_u8 const key = input[n_whole];
    shin_u8 shift = 0;
    for (shin_u32 i = 0; i < n_remainder; i++) {
      SHIN_DBG_ASSERT((shift < 8));
      shin_u8 const code = (key >> shift) & 0x3wbu;

      shin_u32 out = 0;
      switch (code) {
      case 0: {
        out = 0;
        in_offset += 0;
        break;
      }
      case 1: {
        out = input[header_size + in_offset];
        in_offset += 1;
        break;
      }
      case 2: {
        memcpy(&out, &input[header_size + in_offset], 2);
        in_offset += 2;
        break;
      }
      case 3: {
        memcpy(&out, &input[header_size + in_offset], 4);
        in_offset += 4;
        break;
      }
      default: {
        SHIN_DBG_ASSERT((false));
        break;
      }
      }

      output[out_offset + i] = out;
      shift += 2;
    }
  }

  *n_bytes_read = header_size + in_offset;
  return ShinStatus_Ok;
}

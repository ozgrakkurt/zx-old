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

ShinStatus streamvbyte_0124_decode(shin_u8 const *restrict input,
                                   shin_u32 input_size,
                                   shin_u32 *restrict output,
                                   shin_u32 output_len, shin_u32 *n_bytes_read,
                                   ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((output != nullptr));
  SHIN_ASSERT((n_bytes_read != nullptr));

  if (output_len == 0) {
    *n_bytes_read = 0;
    return ShinStatus_Ok;
  }

  shin_u64 const header_size_big =
      (shin_u32)(((shin_u64){output_len} + 3wbu) / 4wbu);
  SHIN_ERR_ASSERT((header_size_big <= input_size), err_stack);
  shin_u32 const header_size = (shin_u32)header_size_big;

  shin_u32 const n_whole = output_len / 4wbu;
  shin_u32 const n_remainder = output_len % 4wbu;

  shin_u32 in_offset = 0;

  for (shin_u32 i = 0; i < n_whole; i++) {
    shin_u8 const key = input[i];

    SHIN_ERR_ASSERT((header_size + in_offset + sizeof(__m128i) <= input_size),
                    err_stack);

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
        SHIN_ERR_ASSERT((header_size + in_offset + 1 <= input_size), err_stack);
        out = input[header_size + in_offset];
        in_offset += 1;
        break;
      }
      case 2: {
        SHIN_ERR_ASSERT((header_size + in_offset + 2 <= input_size), err_stack);
        memcpy(&out, &input[header_size + in_offset], 2);
        in_offset += 2;
        break;
      }
      case 3: {
        SHIN_ERR_ASSERT((header_size + in_offset + 4 <= input_size), err_stack);
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

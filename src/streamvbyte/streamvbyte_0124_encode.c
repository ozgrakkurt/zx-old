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

#include "./streamvbyte_shuffle_tables_0124_encode.h"

ShinStatus streamvbyte_0124_encode(shin_u32 const *restrict input,
                                   shin_u32 input_len, shin_u8 *restrict output,
                                   shin_u32 output_size,
                                   shin_u32 *n_bytes_written,
                                   ShinErrStack *err_stack) {
  SHIN_ASSERT((input != nullptr));
  SHIN_ASSERT((output != nullptr));
  SHIN_ASSERT((n_bytes_written != nullptr));

  shin_u32 bound = 0;
  SHIN_ERR_TRY((streamvbyte_compress_bound(input_len, &bound, err_stack)),
               err_stack);
  SHIN_ERR_ASSERT((output_size >= bound), err_stack);

  if (input_len == 0) {
    *n_bytes_written = 0;
    return ShinStatus_Ok;
  }

  shin_u32 const n_whole = input_len / 4;
  shin_u32 const n_remainder = input_len % 4;
  shin_u32 const header_size =
      (shin_u32)(((shin_u64){input_len} + 3wbu) / 4wbu);

  shin_u32 out_offset = header_size;

  if (n_whole > 0) {
    __m128i const ones = _mm_set1_epi32(0x01010101);
    __m128i const gather_bits = _mm_set1_epi32(0x08040102);
    __m128i const code_table =
        _mm_set_epi32(0x03030303, 0x03030303, 0x03030303, 0x02020100);
    __m128i const gather_bytes = _mm_set_epi32(0, 0, 0x0D090501, 0x0D090501);
    __m128i const aggregators = _mm_set_epi32(0, 0, 0x01010101, 0x10400104);

    for (shin_u32 i = 0; i < n_whole; i++) {
      __m128i const in = _mm_loadu_si128((__m128i const *)&input[4 * i]);

      __m128i const m0 = _mm_madd_epi16(_mm_min_epu8(in, ones), gather_bits);
      __m128i const m1 = _mm_madd_epi16(
          _mm_shuffle_epi8(_mm_shuffle_epi8(code_table, m0), gather_bytes),
          aggregators);

      shin_u8 const code = (shin_u8)_mm_extract_epi8(m1, 1);
      output[i] = code;

      shin_u8 const length =
          STREAMVBYTE_SHUFFLE_TABLES_0124_ENCODE_LENGTH_TABLE[code];

      __m128i const shuffle = _mm_loadu_si128(
          (__m128i const
               *)&STREAMVBYTE_SHUFFLE_TABLES_0124_ENCODE_SHUFFLE_TABLE[code *
                                                                       16]);
      __m128i const out = _mm_shuffle_epi8(in, shuffle);
      _mm_storeu_si128((__m128i *)&output[out_offset], out);
      out_offset += length;
    }
  }

  if (n_remainder > 0) {
    shin_u32 const in_offset = n_whole * 4;
    shin_u8 key = 0;
    shin_u8 shift = 0;
    for (shin_u32 i = 0; i < n_remainder; i++) {
      SHIN_DBG_ASSERT((shift < 8));

      shin_u32 const in = input[in_offset + i];

      if (in == 0wbu) {
        // don't touch the key since key is already zeroed out
        // don't write anything to output
      } else if (in < ((shin_u32){1wbu} << 8)) {
        key |= ((shin_u8){1wbu} << shift);
        output[out_offset] = (shin_u8)in;
        out_offset += 1;
      } else if (in < ((shin_u32){1wbu} << 16)) {
        key |= ((shin_u8){2wbu} << shift);
        memcpy(&output[out_offset], &in, 2);
        out_offset += 2;
      } else {
        key |= ((shin_u8){3wbu} << shift);
        memcpy(&output[out_offset], &in, 4);
        out_offset += 4;
      }

      shift += 2;
    }
    output[n_whole] = key;
  }

  constexpr shin_u8 PADDING = 16;
  memset(&output[out_offset], 0, PADDING);
  *n_bytes_written = out_offset + PADDING;

  return ShinStatus_Ok;
}

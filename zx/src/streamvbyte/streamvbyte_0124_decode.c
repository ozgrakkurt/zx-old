#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE4_1__
#error "sse4.1 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

#include <streamvbyte/streamvbyte.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>

#include "./shuffle_tables/streamvbyte_shuffle_tables_0124_decode.h"

ZXStatus streamvbyte_0124_decode(zx_u8 const *restrict input, zx_u32 input_size,
                                 zx_u32 *restrict output, zx_u32 output_len,
                                 zx_u32 *n_bytes_read, ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((output != nullptr));
  ZX_ASSERT((n_bytes_read != nullptr));

  if (output_len == 0) {
    ZX_FUZZ_MARKER;
    *n_bytes_read = 0;
    return ZXStatus_Ok;
  }

  zx_u64 const header_size_big = (zx_u32)(((zx_u64){output_len} + 3wbu) / 4wbu);
  ZX_ERR_ASSERT((header_size_big <= input_size), err_stack);
  zx_u32 const header_size = (zx_u32)header_size_big;

  zx_u32 const n_whole = output_len / 4wbu;
  zx_u32 const n_remainder = output_len % 4wbu;

  zx_u32 in_offset = header_size;

  if (n_whole > 0) {
    ZX_FUZZ_MARKER;
    typedef unsigned long regt;
    static_assert(sizeof(regt) == sizeof(zx_u64));

    typedef unsigned small_regt;
    static_assert(sizeof(small_regt) == sizeof(zx_u32));

    regt out = (regt)output;
    regt in_head = (regt)input;
    regt in_data = (regt)&input[in_offset];
    regt const in_end = (regt)&input[input_size];
    small_regt n_iter = n_whole;

    __asm__ volatile(
        "0:\n\t"                                    // main decode loop
        "movzbq (%[in_head]), %%r9\n\t"             // get code
        "movzbq (%[length_table], %%r9), %%r10\n\t" // get len
        "movq %[in_data], %%r11\n\t"
        "addq %%r10, %%r11\n\t"     // calculate end pos of our read
        "cmpq %%r11, %[in_end]\n\t" // check we are out of bounds
        "jb 1f\n\t" // early exit if end idx of the read goes over input size
        "vmovdqu (%[in_data]), %%xmm0\n\t"             // load from input
        "shlq $4, %%r9\n\t"                            // prep shuffle offset
        "vmovdqu (%[shuffle_table], %%r9), %%xmm1\n\t" // load shuffle
        "vpshufb %%xmm1, %%xmm0, %%xmm0\n\t"
        "vmovdqu %%xmm0, (%[out])\n\t"
        "movq %%r11, %[in_data]\n\t"
        "incq %[in_head]\n\t"
        "addq $16, %[out]\n\t"
        "decl %[n_iter]\n\t" // check loop counter
        "jnz 0b\n\t"
        "1:\n\t" // marker for early exiting
        : [in_data] "+r"(in_data), [in_head] "+r"(in_head), [out] "+r"(out),
          [n_iter] "+r"(n_iter)
        : [in_end] "r"(in_end),
          [length_table] "r"(
              STREAMVBYTE_SHUFFLE_TABLES_0124_DECODE_LENGTH_TABLE),
          [shuffle_table] "r"(
              STREAMVBYTE_SHUFFLE_TABLES_0124_DECODE_SHUFFLE_TABLE)
        : "xmm0", "xmm1", "r9", "r10", "r11", "cc", "memory");

    // we should have processed to the end unless we hit a problem
    ZX_ERR_ASSERT((out == ((regt)&output[n_whole * 4])), err_stack);
    in_offset = (zx_u32)(in_data - (regt)input);
  }

  if (n_remainder > 0) {
    ZX_FUZZ_MARKER;
    zx_u32 const out_offset = n_whole * 4;
    zx_u8 const key = input[n_whole];
    zx_u8 shift = 0;
    for (zx_u32 i = 0; i < n_remainder; i++) {
      ZX_DBG_ASSERT((shift < 8));
      zx_u8 const code = (key >> shift) & 0x3wbu;

      zx_u32 out = 0;
      switch (code) {
      case 0: {
        out = 0;
        in_offset += 0;
        break;
      }
      case 1: {
        ZX_ERR_ASSERT((in_offset + 1 <= input_size), err_stack);
        out = input[in_offset];
        in_offset += 1;
        break;
      }
      case 2: {
        ZX_ERR_ASSERT((in_offset + 2 <= input_size), err_stack);
        __builtin_memcpy(&out, &input[in_offset], 2);
        in_offset += 2;
        break;
      }
      case 3: {
        ZX_ERR_ASSERT((in_offset + 4 <= input_size), err_stack);
        __builtin_memcpy(&out, &input[in_offset], 4);
        in_offset += 4;
        break;
      }
      default: {
        ZX_DBG_ASSERT((false));
        break;
      }
      }

      output[out_offset + i] = out;
      shift += 2;
    }
  }

  *n_bytes_read = in_offset;
  return ZXStatus_Ok;
}

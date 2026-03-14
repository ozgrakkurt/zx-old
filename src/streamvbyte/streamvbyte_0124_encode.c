#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __AVX2__
#error "avx2 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

#include <streamvbyte/streamvbyte.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>

#include "./shuffle_tables/streamvbyte_shuffle_tables_0124_encode.h"

ZXStatus streamvbyte_0124_encode(zx_u32 const *restrict input, zx_u32 input_len,
                                 zx_u8 *restrict output, zx_u32 output_size,
                                 zx_u32 *n_bytes_written,
                                 ZXErrStack *err_stack) {
  ZX_ASSERT((input != nullptr));
  ZX_ASSERT((output != nullptr));
  ZX_ASSERT((n_bytes_written != nullptr));

  zx_u32 bound = 0;
  ZX_ERR_TRY((streamvbyte_compress_bound(input_len, &bound, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((output_size >= bound), err_stack);

  if (input_len == 0) {
    ZX_FUZZ_MARKER;
    *n_bytes_written = 0;
    return ZXStatus_Ok;
  }

  zx_u32 const n_whole = input_len / 4;
  zx_u32 const n_remainder = input_len % 4;
  zx_u32 const header_size = (zx_u32)(((zx_u64){input_len} + 3wbu) / 4wbu);

  zx_u32 out_offset = header_size;

  if (n_whole > 0) {
    ZX_FUZZ_MARKER;
    typedef unsigned long regt;
    static_assert(sizeof(regt) == sizeof(zx_u64));

    typedef unsigned small_regt;
    static_assert(sizeof(small_regt) == sizeof(zx_u32));

    small_regt const ones = 0x01010101;
    small_regt const gather_bits = 0x08040102;
    small_regt const code_table[4] = {0x02020100, 0x03030303, 0x03030303,
                                      0x03030303};
    small_regt const gather_bytes[4] = {0x0D090501, 0x0D090501, 0, 0};
    small_regt const aggregators[4] = {0x10400104, 0x01010101, 0, 0};

    regt out_data = (regt)&output[out_offset];
    regt out_head = (regt)output;
    regt in = (regt)input;
    small_regt n_iter = n_whole;

    __asm__ volatile(
        "vmovd %[ones], %%xmm0\n\t" // load ones
        "vpbroadcastd %%xmm0, %%xmm0\n\t"
        "vmovd %[gather_bits], %%xmm1\n\t" // load gather_bits
        "vpbroadcastd %%xmm1, %%xmm1\n\t"
        "vmovdqu %[code_table], %%xmm2\n\t"   // load code_table
        "vmovdqu %[gather_bytes], %%xmm3\n\t" // load gather_bytes
        "vmovdqu %[aggregators], %%xmm4\n\t"  // load aggregators
        "0:\n\t"                              // main encode loop
        "vmovdqu (%[input]), %%xmm5\n\t"      // load from input
        "vpminub %%xmm0, %%xmm5, %%xmm6\n\t"
        "vpmaddwd %%xmm1, %%xmm6, %%xmm6\n\t" // m0
        "vpshufb %%xmm6, %%xmm2, %%xmm6\n\t"
        "vpshufb %%xmm3, %%xmm6, %%xmm6\n\t"
        "vpmaddwd %%xmm4, %%xmm6, %%xmm6\n\t" // m1
        "vpextrb $1, %%xmm6, %%r9\n\t"        // code
        "movb %%r9b, (%[out_head])\n\t"       // write code to output
        "movq %%r9, %%r10\n\t"
        "shlq $4, %%r10\n\t"                            // prep shuffle offset
        "vmovdqu (%[shuffle_table], %%r10), %%xmm7\n\t" // load shuffle
        "movzbq (%[length_table], %%r9), %%r11\n\t"     // load length
        "vpshufb %%xmm7, %%xmm5, %%xmm5\n\t"            // do shuffle
        "vmovdqu %%xmm5, (%[out_data])\n\t"             // store to output
        "addq %%r11, %[out_data]\n\t"                   // increment out offset
        "addq $16, %[input]\n\t"
        "incq %[out_head]\n\t"
        "decl %[n_iter]\n\t" // check loop counter
        "jnz 0b\n\t"
        : [out_data] "+r"(out_data), [out_head] "+r"(out_head),
          [input] "+r"(in), [n_iter] "+r"(n_iter)
        : [ones] "r"(ones), [gather_bits] "r"(gather_bits),
          [code_table] "m"(code_table), [gather_bytes] "m"(gather_bytes),
          [aggregators] "m"(aggregators),
          [length_table] "r"(
              STREAMVBYTE_SHUFFLE_TABLES_0124_ENCODE_LENGTH_TABLE),
          [shuffle_table] "r"(
              STREAMVBYTE_SHUFFLE_TABLES_0124_ENCODE_SHUFFLE_TABLE)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "r9",
          "r10", "r11", "cc", "memory");

    out_offset = (zx_u32)(out_data - (regt)output);
  }

  if (n_remainder > 0) {
    ZX_FUZZ_MARKER;
    zx_u32 const in_offset = n_whole * 4;
    zx_u8 key = 0;
    zx_u8 shift = 0;
    for (zx_u32 i = 0; i < n_remainder; i++) {
      ZX_DBG_ASSERT((shift < 8));

      zx_u32 const in = input[in_offset + i];

      if (in == 0wbu) {
        // don't touch the key since key is already zeroed out
        // don't write anything to output
      } else if (in < ((zx_u32){1wbu} << 8)) {
        key |= ((zx_u8){1wbu} << shift);
        output[out_offset] = (zx_u8)in;
        out_offset += 1;
      } else if (in < ((zx_u32){1wbu} << 16)) {
        key |= ((zx_u8){2wbu} << shift);
        __builtin_memcpy(&output[out_offset], &in, 2);
        out_offset += 2;
      } else {
        key |= ((zx_u8){3wbu} << shift);
        __builtin_memcpy(&output[out_offset], &in, 4);
        out_offset += 4;
      }

      shift += 2;
    }
    output[n_whole] = key;
  }

  constexpr zx_u8 PADDING = 16;
  __builtin_memset(&output[out_offset], 0, PADDING);
  ZX_ASSERT((out_offset + PADDING <= bound));

  *n_bytes_written = out_offset + PADDING;

  return ZXStatus_Ok;
}

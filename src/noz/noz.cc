#include <noz/noz.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_int.h>
#include <zx/zx_vec.h>

constexpr zx_u64 NOZ_UNROLL_ = 4;
constexpr zx_u64 NOZ_VEC_SIZE_ = 64; // avx512
constexpr zx_u64 NOZ_STEP_SIZE_ = NOZ_UNROLL_ * NOZ_VEC_SIZE_;

zx_u64 noz_compress_bound(zx_u64 input_size) {
  zx_u64 const n_chunks =
      (zx_chk_add_u64(input_size, NOZ_STEP_SIZE_) - 1) / NOZ_STEP_SIZE_;
  zx_u64 const header_size =
      zx_chk_mul_u64(zx_chk_mul_u64(n_chunks, sizeof(zx_u64)), NOZ_UNROLL_);
  return zx_chk_add_u64(header_size, input_size);
}

static inline zx_u64 noz_make_select_mask_(zx_u64 i, zx_u64 input_size) {
  constexpr zx_u64 ONES = ~(zx_u64){0};
  return ONES >>
         __builtin_elementwise_sub_sat(
             (zx_u64){64}, __builtin_elementwise_sub_sat(input_size, i));
}

zx_u64 noz_compress(zx_u8 const *__restrict input, zx_u64 input_size,
                    zx_u8 *__restrict output, zx_u64 output_size) {
  ZX_ASSERT((output_size >= noz_compress_bound(input_size)));

  zx_u64 out_offset = 0;

  zx_v512_u8 const zeros = zx_v512_setzero_u8();
  for (zx_u64 i = 0; i < input_size; i += NOZ_STEP_SIZE_) {
    zx_u64 const offset0 = i;
    zx_u64 const offset1 = i + NOZ_VEC_SIZE_;
    zx_u64 const offset2 = i + NOZ_VEC_SIZE_ * 2;
    zx_u64 const offset3 = i + NOZ_VEC_SIZE_ * 3;

    zx_u64 const load_mask0 = noz_make_select_mask_(offset0, input_size);
    zx_u64 const load_mask1 = noz_make_select_mask_(offset1, input_size);
    zx_u64 const load_mask2 = noz_make_select_mask_(offset2, input_size);
    zx_u64 const load_mask3 = noz_make_select_mask_(offset3, input_size);

    zx_v512_u8 const in0 =
        zx_v512_mask_loadu_u8(zeros, load_mask0, &input[offset0]);
    zx_v512_u8 const in1 =
        zx_v512_mask_loadu_u8(zeros, load_mask1, &input[offset1]);
    zx_v512_u8 const in2 =
        zx_v512_mask_loadu_u8(zeros, load_mask2, &input[offset2]);
    zx_v512_u8 const in3 =
        zx_v512_mask_loadu_u8(zeros, load_mask3, &input[offset3]);

    zx_u64 const store_mask0 = ~zx_v512_cmpeq_mask_u8(in0, zeros);
    zx_u64 const store_mask1 = ~zx_v512_cmpeq_mask_u8(in1, zeros);
    zx_u64 const store_mask2 = ~zx_v512_cmpeq_mask_u8(in2, zeros);
    zx_u64 const store_mask3 = ~zx_v512_cmpeq_mask_u8(in3, zeros);

    __builtin_memcpy(&output[out_offset], &store_mask0, sizeof(zx_u64));
    __builtin_memcpy(&output[out_offset + 8], &store_mask1, sizeof(zx_u64));
    __builtin_memcpy(&output[out_offset + 16], &store_mask2, sizeof(zx_u64));
    __builtin_memcpy(&output[out_offset + 24], &store_mask3, sizeof(zx_u64));

    zx_u8 const bc0 = zx_popcnt_u64(store_mask0);
    zx_u8 const bc1 = zx_popcnt_u64(store_mask1);
    zx_u8 const bc2 = zx_popcnt_u64(store_mask2);
    zx_u8 const bc3 = zx_popcnt_u64(store_mask3);

    zx_v512_mask_compress_storeu_u8(&output[out_offset + 32], store_mask0, in0);
    zx_v512_mask_compress_storeu_u8(&output[out_offset + 32 + bc0], store_mask1,
                                    in1);
    zx_v512_mask_compress_storeu_u8(&output[out_offset + 32 + bc0 + bc1],
                                    store_mask2, in2);
    zx_v512_mask_compress_storeu_u8(&output[out_offset + 32 + bc0 + bc1 + bc2],
                                    store_mask3, in3);

    out_offset += 32 + bc0 + bc1 + bc2 + bc3;
  }

  return out_offset;
}

zx_u64 noz_decompress(zx_u8 const *__restrict input, zx_u64 input_size,
                      zx_u8 *__restrict output, zx_u64 output_size) {
  zx_u64 in_offset = 0;

  zx_v512_u8 const zeros = zx_v512_setzero_u8();
  for (zx_u64 i = 0; i < input_size; i += NOZ_STEP_SIZE_) {
    zx_u64 load_mask0;
    __builtin_memcpy(&load_mask0, &input[i], sizeof(zx_u64));
    zx_u64 load_mask1;
    __builtin_memcpy(&load_mask1, &input[i + 8], sizeof(zx_u64));
    zx_u64 load_mask2;
    __builtin_memcpy(&load_mask2, &input[i + 16], sizeof(zx_u64));
    zx_u64 load_mask3;
    __builtin_memcpy(&load_mask3, &input[i + 24], sizeof(zx_u64));

    zx_u8 const bc0 = zx_popcnt_u64(load_mask0);
    zx_u8 const bc1 = zx_popcnt_u64(load_mask1);
    zx_u8 const bc2 = zx_popcnt_u64(load_mask2);
    zx_u8 const bc3 = zx_popcnt_u64(load_mask3);

    zx_v512_u8 const in0 =
        zx_v512_mask_loadu_u8(zeros, load_mask0, &input[in_offset + bc0]);
    zx_v512_u8 const in1 =
        zx_v512_mask_loadu_u8(zeros, load_mask1, &input[in_offset + bc1]);
    zx_v512_u8 const in2 =
        zx_v512_mask_loadu_u8(zeros, load_mask2, &input[in_offset + bc2]);
    zx_v512_u8 const in3 =
        zx_v512_mask_loadu_u8(zeros, load_mask3, &input[in_offset + bc3]);

    in_offset += 32 + bc0 + bc1 + bc2 + bc3;

    zx_u64 const offset0 = i;
    zx_u64 const offset1 = i + NOZ_VEC_SIZE_;
    zx_u64 const offset2 = i + NOZ_VEC_SIZE_ * 2;
    zx_u64 const offset3 = i + NOZ_VEC_SIZE_ * 3;

    zx_u64 const store_mask0 = noz_make_select_mask_(offset0, output_size);
    zx_u64 const store_mask1 = noz_make_select_mask_(offset1, output_size);
    zx_u64 const store_mask2 = noz_make_select_mask_(offset2, output_size);
    zx_u64 const store_mask3 = noz_make_select_mask_(offset3, output_size);

    zx_v512_mask_storeu_u8(&output[offset0], store_mask0, in0);
    zx_v512_mask_storeu_u8(&output[offset1], store_mask1, in1);
    zx_v512_mask_storeu_u8(&output[offset2], store_mask2, in2);
    zx_v512_mask_storeu_u8(&output[offset3], store_mask3, in3);
  }

  return in_offset;
}

#include <noz/noz.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

constexpr zx_u64 NOZ_VEC_SIZE_ = 64; // avx512

zx_u64 noz_compress_bound(zx_u64 input_size) {
  ZX_FUZZ_MARKER;
  zx_u64 const n_chunks =
      (zx_chk_add_u64(input_size, NOZ_VEC_SIZE_) - 1) / NOZ_VEC_SIZE_;
  zx_u64 const header_size = zx_chk_mul_u64(n_chunks, sizeof(zx_u64));
  return zx_chk_add_u64(header_size, input_size);
}

typedef unsigned long regt;
static_assert(sizeof(regt) == sizeof(void *));

typedef zx_u8 vect __attribute__((vector_size(64)));

zx_u64 noz_compress(zx_u8 const *__restrict input, zx_u64 input_size,
                    zx_u8 *__restrict output, zx_u64 output_size) {
  ZX_ASSERT((output_size >= noz_compress_bound(input_size)));

  if (input_size == 0) {
    ZX_FUZZ_MARKER;
    return 0;
  }
  ZX_FUZZ_MARKER;

  vect zeros;
  __asm__("vxorps %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=v"(zeros));

  zx_u64 const out_start = (zx_u64)output;

  zx_u64 in_offset = 0;
  for (; in_offset + 64 <= input_size; in_offset += 64) {
    ZX_FUZZ_MARKER;
    regt n_w;
    __asm__("vmovdqu8 %[input], %%zmm0\n\t"
            "vpcmpub $4, %%zmm0, %[zeros], %%k1\n\t"
            "kmovq %%k1, %[out0]\n\t"
            "kmovq %%k1, %[n_w]\n\t"
            "vpcompressb %%zmm0, %%zmm1 %{%%k1%}\n\t"
            "vmovdqu8 %%zmm1, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=m"(*(zx_u8(*)[8])output),
              [out1] "=m"(*(zx_u8(*)[64])(output + 8)), [n_w] "=r"(n_w)
            : [input] "m"(*(zx_u8 const(*)[64])input), [zeros] "v"(zeros)
            : "zmm0", "zmm1", "k1");

    input += 64;
    output += n_w + sizeof(zx_u64);
  }

  regt const remainder = input_size - in_offset;
  if (remainder > 0) {
    ZX_FUZZ_MARKER;
    register regt const loadmask __asm__("k2") =
        __builtin_ia32_bzhi_di(~(regt){0}, remainder);
    regt n_w;
    __asm__("vmovdqu8 %[input], %%zmm0 %{%[loadmask]%}%{z%}\n\t"
            "vpcmpub $4, %%zmm0, %[zeros], %%k1\n\t"
            "kmovq %%k1, %[out0]\n\t"
            "kmovq %%k1, %[n_w]\n\t"
            "vpcompressb %%zmm0, %%zmm1 %{%%k1%}\n\t"
            "vmovdqu8 %%zmm1, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=m"(*(zx_u8(*)[8])output),
              [out1] "=m"(*(zx_u8(*)[64])(output + 8)), [n_w] "=r"(n_w)
            : [input] "m"(*(zx_u8 const(*)[64])input), [zeros] "v"(zeros),
              [loadmask] "k"(loadmask)
            : "zmm0", "zmm1", "k1");

    output += n_w + sizeof(zx_u64);
  }

  return (zx_u64)output - out_start;
}

zx_u64 noz_decompress(zx_u8 const *__restrict input, zx_u8 *__restrict output,
                      zx_u64 output_size) {
  if (output_size == 0) {
    ZX_FUZZ_MARKER;
    return 0;
  }
  ZX_FUZZ_MARKER;

  zx_u64 const input_start = (zx_u64)input;

  zx_u64 out_offset = 0;
  for (; out_offset + 64 <= output_size; out_offset += 64) {
    ZX_FUZZ_MARKER;
    register regt control __asm__("k1");
    __builtin_memcpy(&control, input, sizeof(regt));

    __asm__("vpexpandb %[input], %%zmm0 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm0, %[output]\n\t"
            : [output] "=m"(*(zx_u8(*)[64])output)
            : [input] "m"(*(zx_u8 const(*)[64])(input + sizeof(zx_u64))),
              [control] "k"(control)
            : "zmm0");

    output += 64;
    input += zx_popcnt_u64(control) + sizeof(zx_u64);
  }

  regt const remainder = output_size - out_offset;
  if (remainder > 0) {
    ZX_FUZZ_MARKER;
    register regt control __asm__("k1");
    __builtin_memcpy(&control, input, sizeof(regt));

    register regt const storemask __asm__("k2") =
        __builtin_ia32_bzhi_di(~(regt){0}, remainder);

    __asm__("vpexpandb %[input], %%zmm0 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm0, %[output] %{%[storemask]%}\n\t"
            : [output] "=m"(*(zx_u8(*)[64])output)
            : [input] "m"(*(zx_u8 const(*)[64])(input + sizeof(zx_u64))),
              [control] "k"(control), [storemask] "k"(storemask)
            : "zmm0");

    input += zx_popcnt_u64(control) + sizeof(zx_u64);
  }

  return (zx_u64)input - input_start;
}

ZXStatus noz_decompress_safe(zx_u8 const *__restrict input, zx_u64 input_size,
                             zx_u8 *__restrict output, zx_u64 output_size,
                             zx_u64 *n_read, ZXErrStack *err_stack) {
  if (output_size == 0) {
    ZX_FUZZ_MARKER;
    *n_read = 0;
    return ZXStatus::Ok;
  }
  ZX_FUZZ_MARKER;

  zx_u64 const input_start = (zx_u64)input;
  zx_u64 const input_end = zx_chk_add_u64(input_start, input_size);

  zx_u64 out_offset = 0;
  for (; out_offset + 64 <= output_size; out_offset += 64) {
    ZX_FUZZ_MARKER;

    zx_u64 const control_end = ((zx_u64)input) + sizeof(zx_u64);
    ZX_ERR_ASSERT((control_end <= input_end), err_stack);

    register regt control __asm__("k1");
    __builtin_memcpy(&control, input, sizeof(regt));

    zx_u64 const step = zx_popcnt_u64(control) + sizeof(zx_u64);
    zx_u64 const step_end = ((zx_u64)input) + step;
    ZX_ERR_ASSERT((step_end <= input_end), err_stack);

    __asm__("vpexpandb %[input], %%zmm0 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm0, %[output]\n\t"
            : [output] "=m"(*(zx_u8(*)[64])output)
            : [input] "m"(*(zx_u8 const(*)[64])(input + sizeof(zx_u64))),
              [control] "k"(control)
            : "zmm0");

    output += 64;
    input += step;
  }

  regt const remainder = output_size - out_offset;
  if (remainder > 0) {
    ZX_FUZZ_MARKER;

    zx_u64 const control_end = ((zx_u64)input) + sizeof(zx_u64);
    ZX_ERR_ASSERT((control_end <= input_end), err_stack);

    register regt control __asm__("k1");
    __builtin_memcpy(&control, input, sizeof(regt));

    zx_u64 const step = zx_popcnt_u64(control) + sizeof(zx_u64);
    zx_u64 const step_end = ((zx_u64)input) + step;
    ZX_ERR_ASSERT((step_end <= input_end), err_stack);

    register regt const storemask __asm__("k2") =
        __builtin_ia32_bzhi_di(~(regt){0}, remainder);

    __asm__("vpexpandb %[input], %%zmm0 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm0, %[output] %{%[storemask]%}\n\t"
            : [output] "=m"(*(zx_u8(*)[64])output)
            : [input] "m"(*(zx_u8 const(*)[64])(input + sizeof(zx_u64))),
              [control] "k"(control), [storemask] "k"(storemask)
            : "zmm0");

    input += step;
  }

  *n_read = (zx_u64)input - input_start;
  return ZXStatus::Ok;
}

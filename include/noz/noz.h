#ifndef NOZ_H
#define NOZ_H

#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_vec.h>

namespace noz {

enum class [[nodiscard]] CompressBoundStatus : zx::u8_c {
  Ok = 0,
  Nullptr,
  IntegerOverflow,
};

static inline CompressBoundStatus compress_bound(zx::u64 input_size,
                                                        zx::u64 *bound) {
  if (bound == nullptr) {
    return CompressBoundStatus::Nullptr;
  }

  bool overflow = false;

  zx::u64 const n_chunks = zx::div_ceil_u64(input_size, 64, &overflow);

  zx::u64 const header_size = zx::mul_u64(n_chunks, sizeof(zx::u64), &overflow);

  zx::u64 const data_size = zx::mul_u64(n_chunks, 64, &overflow);

  *bound = zx::add_u64(header_size, data_size, &overflow);

  if (overflow) {
    return CompressBoundStatus::IntegerOverflow;
  }

  return CompressBoundStatus::Ok;
}

enum class [[nodiscard]] CompressStatus : zx::u8_c {
  Ok = 0,
  Nullptr,
  InputOutputOverlap,
  BoundFail,
  OutputTooShort,
};

static inline CompressStatus compress(ZXSlice<zx::u8> input,
                                             ZXSliceMut<zx::u8> output,
                                             zx::u64 *n_written) {
  if (n_written == nullptr) {
    return CompressStatus::Nullptr;
  }

  if (input.is_overlapping(output.as_const())) {
    return CompressStatus::InputOutputOverlap;
  }

  zx::u64 bound = zx::U64_MAX;
  if (CompressBoundStatus::Ok != compress_bound(input.len(), &bound)) {
    return CompressStatus::BoundFail;
  }

  if (output.len() < bound) {
    return CompressStatus::OutputTooShort;
  }

  if (input.len() == 0) {
    *n_written = 0;
    return CompressStatus::Ok;
  }

  zx::v512_u8 zeros;
  __asm__("vxorps %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=&v"(zeros));

  zx::u64 in_offset = 0;
  zx::u64 out_offset = 0;
  for (; zx::add_unsafe_u64(in_offset, 64) <= input.len();
       in_offset = zx::add_unsafe_u64(in_offset, 64)) {
    zx::u64 const data_offset = zx::add_unsafe_u64(out_offset, sizeof(zx::u64));
    zx::u64_c n_w;
    __asm__("vmovdqu8 %[input], %%zmm31\n\t"
            "vpcmpub $4, %%zmm31, %[zeros], %%k7\n\t"
            "kmovq %%k7, %[out0]\n\t"
            "kmovq %%k7, %[n_w]\n\t"
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vmovdqu8 %%zmm30, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=&m"(*(zx::u8(*)[8]) & output.ptr()[out_offset]),
              [out1] "=&m"(*(zx::u8(*)[64]) & output.ptr()[data_offset]),
              [n_w] "=&r"(n_w)
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[in_offset]),
              [zeros] "v"(zeros)
            : "zmm30", "zmm31", "k7");

    out_offset = zx::add_unsafe_u64(data_offset, n_w);
  }

  zx::u64 const remainder = zx::sub_unsafe_u64(input.len(), in_offset);
  if (remainder > 0) {
    register zx::u64_c const loadmask __asm__("k6") =
        __builtin_ia32_bzhi_di(~(zx::u64){0}, remainder);
    zx::u64 const data_offset = zx::add_unsafe_u64(out_offset, sizeof(zx::u64));
    zx::u64_c n_w;
    __asm__("vmovdqu8 %[input], %%zmm31 %{%[loadmask]%}%{z%}\n\t"
            "vpcmpub $4, %%zmm31, %[zeros], %%k7\n\t"
            "kmovq %%k7, %[out0]\n\t"
            "kmovq %%k7, %[n_w]\n\t"
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vmovdqu8 %%zmm30, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=&m"(*(zx::u8(*)[8]) & output.ptr()[out_offset]),
              [out1] "=&m"(*(zx::u8(*)[64]) & output.ptr()[data_offset]),
              [n_w] "=&r"(n_w)
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[in_offset]),
              [zeros] "v"(zeros), [loadmask] "k"(loadmask)
            : "zmm31", "zmm30", "k7");

    out_offset = zx::add_unsafe_u64(data_offset, n_w);
  }

  *n_written = out_offset;
  return CompressStatus::Ok;
}

enum class [[nodiscard]] DecompressStatus : zx::u8_c {
  Ok = 0,
  Nullptr,
  InputOutputOverlap,
  InputTooShort,
};

static inline DecompressStatus
decompress(ZXSlice<zx::u8> input, ZXSliceMut<zx::u8> output, zx::u64 *n_read) {
  if (n_read == nullptr) {
    *n_read = 0;
    return DecompressStatus::Nullptr;
  }

  if (input.is_overlapping(output.as_const())) {
    *n_read = 0;
    return DecompressStatus::InputOutputOverlap;
  }

  if (output.len() == 0) {
    *n_read = 0;
    return DecompressStatus::Ok;
  }

  zx::u64 in_offset = 0;
  zx::u64 out_offset = 0;
  for (; zx::add_unsafe_u64(out_offset, 64) <= output.len();
       out_offset = zx::add_unsafe_u64(out_offset, 64)) {
    zx::u64 const control_end = zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
    if (control_end > input.len()) {
      *n_read = 0;
      return DecompressStatus::InputTooShort;
    }

    register zx::u64_c control __asm__("k7") =
        input.read_unaligned_unsafe<zx::u64>(in_offset);

    zx::u64 const step_end =
        zx::add_unsafe_u64(control_end, zx::popcnt_u64(control));
    if (step_end > input.len()) {
      *n_read = 0;
      return DecompressStatus::InputTooShort;
    }

    __asm__("vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output]\n\t"
            : [output] "=&m"(*(zx::u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control)
            : "zmm31");

    in_offset = step_end;
  }

  zx::u64 const remainder = zx::sub_unsafe_u64(output.len(), out_offset);
  if (remainder > 0) {
    zx::u64 const control_end = zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
    if (control_end > input.len()) {
      *n_read = 0;
      return DecompressStatus::InputTooShort;
    }

    register zx::u64_c control __asm__("k7") =
        input.read_unaligned_unsafe<zx::u64>(in_offset);

    zx::u64 const step_end =
        zx::add_unsafe_u64(control_end, zx::popcnt_u64(control));
    if (step_end > input.len()) {
      *n_read = 0;
      return DecompressStatus::InputTooShort;
    }

    register zx::u64_c const storemask __asm__("k6") =
        __builtin_ia32_bzhi_di(~(zx::u64){0}, remainder);

    __asm__("vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output] %{%[storemask]%}\n\t"
            : [output] "=&m"(*(zx::u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control), [storemask] "k"(storemask)
            : "zmm31");

    in_offset = step_end;
  }

  *n_read = in_offset;
  return DecompressStatus::Ok;
}

} // namespace noz

#endif

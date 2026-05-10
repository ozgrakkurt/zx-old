#ifndef NOZ_H
#define NOZ_H

#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_vec.h>

enum class [[nodiscard]] NozCompressBoundStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  IntegerOverflow,
};

static inline ZXCString
noz_compress_bound_status_to_cstring(NozCompressBoundStatus status) {
  switch (status) {
  case NozCompressBoundStatus::Ok: {
    return ZXCString::init_unsafe("NozCompressBoundStatus::Ok");
  }
  case NozCompressBoundStatus::Nullptr: {
    return ZXCString::init_unsafe("NozCompressBoundStatus::Nullptr");
  }
  case NozCompressBoundStatus::IntegerOverflow: {
    return ZXCString::init_unsafe("NozCompressBoundStatus::IntegerOverflow");
  }
  default: {
    return ZXCString::init_unsafe("NozCompressBoundStatus::Unknown");
  }
  }
}

enum class [[nodiscard]] NozCompressStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  InputOutputOverlap,
  BoundFail,
  OutputTooShort,
};

static inline ZXCString
noz_compress_status_to_cstring(NozCompressStatus status) {
  switch (status) {
  case NozCompressStatus::Ok: {
    return ZXCString::init_unsafe("NozCompressStatus::Ok");
  }
  case NozCompressStatus::Nullptr: {
    return ZXCString::init_unsafe("NozCompressStatus::Nullptr");
  }
  case NozCompressStatus::InputOutputOverlap: {
    return ZXCString::init_unsafe("NozCompressStatus::InputOutputOverlap");
  }
  case NozCompressStatus::BoundFail: {
    return ZXCString::init_unsafe("NozCompressStatus::BoundFail");
  }
  case NozCompressStatus::OutputTooShort: {
    return ZXCString::init_unsafe("NozCompressStatus::OutputTooShort");
  }
  default: {
    return ZXCString::init_unsafe("NozCompressStatus::Unknown");
  }
  }
}

enum class [[nodiscard]] NozDecompressStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  InputOutputOverlap,
  InputTooShort,
};

static inline ZXCString
noz_decompress_status_to_cstring(NozDecompressStatus status) {
  switch (status) {
  case NozDecompressStatus::Ok: {
    return ZXCString::init_unsafe("NozDecompressStatus::Ok");
  }
  case NozDecompressStatus::Nullptr: {
    return ZXCString::init_unsafe("NozDecompressStatus::Nullptr");
  }
  case NozDecompressStatus::InputOutputOverlap: {
    return ZXCString::init_unsafe("NozDecompressStatus::InputOutputOverlap");
  }
  case NozDecompressStatus::InputTooShort: {
    return ZXCString::init_unsafe("NozDecompressStatus::InputTooShort");
  }
  default: {
    return ZXCString::init_unsafe("NozDecompressStatus::Unknown");
  }
  }
}

static inline NozCompressBoundStatus noz_compress_bound(zx_u64 input_size,
                                                        zx_u64 *bound) {
  if (bound == nullptr) {
    return NozCompressBoundStatus::Nullptr;
  }

  bool overflow = false;

  zx_u64 const n_chunks = zx_div_ceil_u64(input_size, 64, &overflow);

  zx_u64 const header_size = zx_mul_u64(n_chunks, sizeof(zx_u64), &overflow);

  zx_u64 const data_size = zx_mul_u64(n_chunks, 64, &overflow);

  *bound = zx_add_u64(header_size, data_size, &overflow);

  if (overflow) {
    return NozCompressBoundStatus::IntegerOverflow;
  }

  return NozCompressBoundStatus::Ok;
}

static inline NozCompressStatus noz_compress(ZXSlice<zx_u8> input,
                                             ZXSliceMut<zx_u8> output,
                                             zx_u64 *n_written) {
  if (n_written == nullptr) {
    return NozCompressStatus::Nullptr;
  }

  if (input.is_overlapping(output.as_const())) {
    return NozCompressStatus::InputOutputOverlap;
  }

  zx_u64 bound = ZX_U64_MAX;
  if (NozCompressBoundStatus::Ok != noz_compress_bound(input.len(), &bound)) {
    return NozCompressStatus::BoundFail;
  }

  if (output.len() < bound) {
    return NozCompressStatus::OutputTooShort;
  }

  if (input.len() == 0) {
    *n_written = 0;
    return NozCompressStatus::Ok;
  }

  zx_v512_u8 zeros;
  __asm__("vxorps %[zeros], %[zeros], %[zeros]\n\t" : [zeros] "=&v"(zeros));

  zx_u64 in_offset = 0;
  zx_u64 out_offset = 0;
  for (; zx_add_unsafe_u64(in_offset, 64) <= input.len();
       in_offset = zx_add_unsafe_u64(in_offset, 64)) {
    zx_u64 const data_offset = zx_add_unsafe_u64(out_offset, sizeof(zx_u64));
    zx_u64_c n_w;
    __asm__("vmovdqu8 %[input], %%zmm31\n\t"
            "vpcmpub $4, %%zmm31, %[zeros], %%k7\n\t"
            "kmovq %%k7, %[out0]\n\t"
            "kmovq %%k7, %[n_w]\n\t"
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vmovdqu8 %%zmm30, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=&m"(*(zx_u8(*)[8]) & output.ptr()[out_offset]),
              [out1] "=&m"(*(zx_u8(*)[64]) & output.ptr()[data_offset]),
              [n_w] "=&r"(n_w)
            : [input] "m"(*(zx_u8 const(*)[64]) & input.ptr()[in_offset]),
              [zeros] "v"(zeros)
            : "zmm30", "zmm31", "k7");

    out_offset = zx_add_unsafe_u64(data_offset, n_w);
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(input.len(), in_offset);
  if (remainder > 0) {
    register zx_u64_c const loadmask __asm__("k6") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);
    zx_u64 const data_offset = zx_add_unsafe_u64(out_offset, sizeof(zx_u64));
    zx_u64_c n_w;
    __asm__("vmovdqu8 %[input], %%zmm31 %{%[loadmask]%}%{z%}\n\t"
            "vpcmpub $4, %%zmm31, %[zeros], %%k7\n\t"
            "kmovq %%k7, %[out0]\n\t"
            "kmovq %%k7, %[n_w]\n\t"
            "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
            "vmovdqu8 %%zmm30, %[out1]\n\t"
            "popcntq %[n_w], %[n_w]\n\t"
            : [out0] "=&m"(*(zx_u8(*)[8]) & output.ptr()[out_offset]),
              [out1] "=&m"(*(zx_u8(*)[64]) & output.ptr()[data_offset]),
              [n_w] "=&r"(n_w)
            : [input] "m"(*(zx_u8 const(*)[64]) & input.ptr()[in_offset]),
              [zeros] "v"(zeros), [loadmask] "k"(loadmask)
            : "zmm31", "zmm30", "k7");

    out_offset = zx_add_unsafe_u64(data_offset, n_w);
  }

  *n_written = out_offset;
  return NozCompressStatus::Ok;
}

static inline NozDecompressStatus
noz_decompress(ZXSlice<zx_u8> input, ZXSliceMut<zx_u8> output, zx_u64 *n_read) {
  if (n_read == nullptr) {
    *n_read = 0;
    return NozDecompressStatus::Nullptr;
  }

  if (input.is_overlapping(output.as_const())) {
    *n_read = 0;
    return NozDecompressStatus::InputOutputOverlap;
  }

  if (output.len() == 0) {
    *n_read = 0;
    return NozDecompressStatus::Ok;
  }

  zx_u64 in_offset = 0;
  zx_u64 out_offset = 0;
  for (; zx_add_unsafe_u64(out_offset, 64) <= output.len();
       out_offset = zx_add_unsafe_u64(out_offset, 64)) {
    zx_u64 const control_end = zx_add_unsafe_u64(in_offset, sizeof(zx_u64));
    if (control_end > input.len()) {
      *n_read = 0;
      return NozDecompressStatus::InputTooShort;
    }

    register zx_u64_c control __asm__("k7") =
        input.read_unaligned_unsafe<zx_u64>(in_offset);

    zx_u64 const step_end =
        zx_add_unsafe_u64(control_end, zx_popcnt_u64(control));
    if (step_end > input.len()) {
      *n_read = 0;
      return NozDecompressStatus::InputTooShort;
    }

    __asm__("vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output]\n\t"
            : [output] "=&m"(*(zx_u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx_u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control)
            : "zmm31");

    in_offset = step_end;
  }

  zx_u64 const remainder = zx_sub_unsafe_u64(output.len(), out_offset);
  if (remainder > 0) {
    zx_u64 const control_end = zx_add_unsafe_u64(in_offset, sizeof(zx_u64));
    if (control_end > input.len()) {
      *n_read = 0;
      return NozDecompressStatus::InputTooShort;
    }

    register zx_u64_c control __asm__("k7") =
        input.read_unaligned_unsafe<zx_u64>(in_offset);

    zx_u64 const step_end =
        zx_add_unsafe_u64(control_end, zx_popcnt_u64(control));
    if (step_end > input.len()) {
      *n_read = 0;
      return NozDecompressStatus::InputTooShort;
    }

    register zx_u64_c const storemask __asm__("k6") =
        __builtin_ia32_bzhi_di(~(zx_u64){0}, remainder);

    __asm__("vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output] %{%[storemask]%}\n\t"
            : [output] "=&m"(*(zx_u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx_u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control), [storemask] "k"(storemask)
            : "zmm31");

    in_offset = step_end;
  }

  *n_read = in_offset;
  return NozDecompressStatus::Ok;
}

#endif

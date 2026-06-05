#ifndef ZX_LEB128_H
#define ZX_LEB128_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXLeb128Status : zx_u8_c {
  Ok = 0,
  OutOfCapacity,
  InvalidInput,
  UnexpectedEndOfInput,
  Nullptr,
};

constexpr zx_u8 ZX_LEB_CONT_BIT = 0x80__wbu;
constexpr zx_u8 ZX_LEB_SIGN_BIT = 0x40__wbu;

static inline zx_u8 zx_leb128_getval_(zx_u8 byte) { return byte & 0x7f__wbu; }

static inline ZXLeb128Status
zx_uleb128_from_u64(zx_u64 num, ZXSliceMut<zx_u8> out, zx_u64 *offset) {
  if (offset == nullptr) {
    return ZXLeb128Status::Nullptr;
  }

  zx_u64 offs = *offset;

  while (true) {
    if (offs == out.len()) {
      *offset = offs;
      return ZXLeb128Status::OutOfCapacity;
    }

    zx_u8 const byte = zx_leb128_getval_((zx_u8)num);
    num = zx_shr_unsafe_u64(num, 7);
    if (num != 0) {
      out.write_unsafe(offs, byte | ZX_LEB_CONT_BIT);
      offs = zx_add_unsafe_u64(offs, 1);
    } else {
      out.write_unsafe(offs, byte);
      offs = zx_add_unsafe_u64(offs, 1);
      *offset = offs;
      return ZXLeb128Status::Ok;
    }
  }
}

static inline ZXLeb128Status zx_uleb128_to_u64(ZXSlice<zx_u8> in,
                                               zx_u64 *offset, zx_u64 *num) {
  if (offset == nullptr || num == nullptr) {
    return ZXLeb128Status::Nullptr;
  }

  zx_u64 offs = *offset;

  zx_u64 out = 0;
  zx_u8 shift = 0;
  for (zx_u8 iter = 0; iter < 10; iter++) {
    if (offs == in.len()) {
      *num = 0;
      *offset = offs;
      return ZXLeb128Status::UnexpectedEndOfInput;
    }

    zx_u8 const byte = in.read_unsafe(offs);
    offs = zx_add_unsafe_u64(offs, 1);
    out |= zx_shl_unsafe_u64(zx_leb128_getval_(byte), shift);
    shift = zx_add_unsafe_u8(shift, 7);
    if ((byte & ZX_LEB_CONT_BIT) == 0) {
      *num = out;
      *offset = offs;
      return ZXLeb128Status::Ok;
    } else if (zx_leb128_getval_(byte) == 0) {
      *num = 0;
      *offset = offs;
      return ZXLeb128Status::InvalidInput;
    }
  }

  *num = 0;
  *offset = offs;
  return ZXLeb128Status::InvalidInput;
}

static inline ZXLeb128Status
zx_sleb128_from_i64(zx_i64 num, ZXSliceMut<zx_u8> out, zx_u64 *offset) {
  if (offset == nullptr) {
    return ZXLeb128Status::Nullptr;
  }

  zx_u64 offs = *offset;

  while (true) {
    if (offs == out.len()) {
      *offset = offs;
      return ZXLeb128Status::OutOfCapacity;
    }

    zx_u8 const byte = zx_leb128_getval_((zx_u8)num);
    num = zx_shr_unsafe_i64(num, 7);
    bool const negative_end = num == -1 && (byte & ZX_LEB_SIGN_BIT) != 0;
    bool const positive_end = num == 0 && (byte & ZX_LEB_SIGN_BIT) == 0;
    if (negative_end || positive_end) {
      out.write_unsafe(offs, byte);
      offs = zx_add_unsafe_u64(offs, 1);
      *offset = offs;
      return ZXLeb128Status::Ok;
    } else {
      out.write_unsafe(offs, byte | ZX_LEB_CONT_BIT);
      offs = zx_add_unsafe_u64(offs, 1);
    }
  }
}

static inline ZXLeb128Status zx_sleb128_to_i64(ZXSlice<zx_u8> in,
                                               zx_u64 *offset, zx_i64 *num) {
  if (offset == nullptr || num == nullptr) {
    return ZXLeb128Status::Nullptr;
  }

  zx_u64 offs = *offset;

  zx_u64 out = 0;
  zx_u8 shift = 0;
  for (zx_u8 iter = 0; iter < 10; iter++) {
    if (offs == in.len()) {
      *num = 0;
      *offset = offs;
      return ZXLeb128Status::UnexpectedEndOfInput;
    }

    zx_u8 const byte = in.read_unsafe(offs);
    offs = zx_add_unsafe_u64(offs, 1);
    out |= zx_shl_unsafe_u64(zx_leb128_getval_(byte), shift);
    shift = zx_add_unsafe_u8(shift, 7);
    if ((byte & ZX_LEB_CONT_BIT) == 0) {
      // sign extend if needed
      bool const is_negative = (byte & ZX_LEB_SIGN_BIT) == ZX_LEB_SIGN_BIT;
      if (shift < 64 && is_negative) {
        out |= zx_shl_unsafe_u64(~zx_u64{0}, shift);
      }
      *num = (zx_i64)out;
      *offset = offs;
      return ZXLeb128Status::Ok;
    } else if (zx_leb128_getval_(byte) == 0) {
      *num = 0;
      *offset = offs;
      return ZXLeb128Status::InvalidInput;
    }
  }

  *num = 0;
  *offset = offs;
  return ZXLeb128Status::InvalidInput;
}

#endif

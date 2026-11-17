#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_def.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

struct ZXFmt {
  // Backing memory
  ZXSliceMut<zx_u8> m_mem;
  // Current write offset;
  zx_u64 m_offset;

  // Create a new formatter with given memory. The formatter will
  // use this memory for writing the characters.
  static inline ZXFmt init(ZXSlice<zx_u8> mem) {
    mem.memset(0);
    return ZXFmt{
        .m_mem = mem,
        .m_offset = 0,
    };
  }

  // Return a slice that points to the characters written so far.
  inline ZXSlice<zx_u8> written() const {
    return this->m_mem.slice(0, this->m_offset);
  }

  inline void reset() {
    this->m_mem.memset(0);
    this->m_offset = 0;
  }

  // Dummy func so we get a compile error if formatting isn't implemented for
  // `T`
  template <typename T> inline void format(T val) {
    static_assert(sizeof(T) == 0);
    (void)val;
  }

  template <> inline void format<ZXSlice<char>>(ZXSlice<char> str) {
    zx_u64 const capacity_left =
        zx_chk_sub_u64(this->m_mem.len(), this->m_offset);
    zx_u64 const n_write = zx_int_min_u64(capacity_left, str.len());
    ZXSliceMut<zx_u8> const w_slice =
        this->m_mem.slice(this->m_offset, n_write);
    w_slice.memcpy_from(str.scast::<ZXSlice<zx_u8>>());
    this->m_offset += n_write;
  }

  template <> inline void format<char const *>(char const *cstr) {
    this->format<ZXSlice<char>>(zx_slice_from_cstr(cstr));
  }

  template <> inline void format<char>(char ch) {
    zx_u64 const capacity_left =
        zx_chk_sub_u64(this->m_mem.len(), this->m_offset);
    if (capacity_left > 0) {
      *this->m_mem.index(this->m_offset) = (zx_u8){ch};
      this->m_offset += 1;
    }
  }

  template <> inline void format<zx_u64>(zx_u64 val) {
    constexpr zx_u64 MAX_NUM_DIGITS = 20;
    char digits[MAX_NUM_DIGITS];

    zx_u64 pos = MAX_NUM_DIGITS;
    do {
      pos--;
      digits[pos] = '0' + (char)(val % 10);
      val /= 10;
    } while (val > 0);
    zx_u64 const len = zx_chk_sub_u64(MAX_NUM_DIGITS, pos);
    this->format<ZXSlice<char>>(ZXSlice<char>::init(digits, len));
  }

  template <> inline void format<zx_u8>(zx_u8 val) {
    this->format<zx_u64>(val);
  }

  template <> inline void format<zx_u16>(zx_u16 val) {
    this->format<zx_u64>(val);
  }

  template <> inline void format<zx_u32>(zx_u32 val) {
    this->format<zx_u64>(val);
  }

  template <> inline void format<zx_i64>(zx_i64 val) {
    if (val >= 0) {
      this->format<zx_u64>((zx_u64)val);
    } else {
      zx_u64 const capacity_left =
          zx_chk_sub_u64(this->m_mem.len(), this->m_offset);
      if (capacity_left > 0) {
        *this->m_mem.index(this->m_offset) = (zx_u8){'-'};
        this->m_offset += 1;
        this->format<zx_u64>((zx_u64)-val);
      }
    }
  }

  template <> inline void format<zx_i8>(zx_i8 val) {
    this->format<zx_i64>(val);
  }

  template <> inline void format<zx_i16>(zx_i16 val) {
    this->format<zx_i64>(val);
  }

  template <> inline void format<zx_i32>(zx_i32 val) {
    this->format<zx_i64>(val);
  }
};

#define ZX_FMT_1_(fmt, v) (fmt)->format((v))
#define ZX_FMT_2_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_1_(fmt, __VA_ARGS__))
#define ZX_FMT_3_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_2_(fmt, __VA_ARGS__))
#define ZX_FMT_4_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_3_(fmt, __VA_ARGS__))
#define ZX_FMT_5_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_4_(fmt, __VA_ARGS__))
#define ZX_FMT_6_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_5_(fmt, __VA_ARGS__))
#define ZX_FMT_7_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_6_(fmt, __VA_ARGS__))
#define ZX_FMT_8_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_7_(fmt, __VA_ARGS__))
#define ZX_FMT_9_(fmt, v, ...)                                                 \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_8_(fmt, __VA_ARGS__))
#define ZX_FMT_10_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_9_(fmt, __VA_ARGS__))
#define ZX_FMT_11_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_10_(fmt, __VA_ARGS__))
#define ZX_FMT_12_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_11_(fmt, __VA_ARGS__))
#define ZX_FMT_13_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_12_(fmt, __VA_ARGS__))
#define ZX_FMT_14_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_13_(fmt, __VA_ARGS__))
#define ZX_FMT_15_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_14_(fmt, __VA_ARGS__))
#define ZX_FMT_16_(fmt, v, ...)                                                \
  (fmt)->format((v)) __VA_OPT__(; ZX_FMT_15_(fmt, __VA_ARGS__))
#define ZX_FMT(fmt, v, ...) ZX_FMT_16_(fmt, v, __VA_ARGS__)

#endif

#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_def.h>
#include <zx/zx_int.h>

struct ZXFmtStr {
  char const *str;
  zx_u32 len;
};

struct ZXFmt {
  char *buf_;
  zx_u32 capacity;
  zx_u32 offset;

  static inline ZXFmt init(char *buf, zx_u32 capacity) {
    return ZXFmt{
        .buf_ = buf,
        .capacity = capacity,
        .offset = 0,
    };
  }

  inline zx_u32 n_written() const { return this->offset; }

  inline char const *buf() const { return this->buf_; }

  inline void reset() { this->offset = 0; }

  template <typename T> inline void format(T val) {
    static_assert(sizeof(T) == 0);
    (void)val;
  }

  template <> inline void format<ZXFmtStr>(ZXFmtStr str) {
    zx_u32 const capacity_left = zx_chk_sub_u32(this->capacity, this->offset);
    zx_u64 const n_write = zx_int_min_u64(capacity_left, str.len);
    __builtin_memcpy(&this->buf_[this->offset], str.str, n_write);
    this->offset += n_write;
  }

  template <> inline void format<char const *>(char const *cstr) {
    this->format<ZXFmtStr>(
        ZXFmtStr{.str = cstr, .len = (zx_u32)__builtin_strlen(cstr)});
  }

  template <> inline void format<char *>(char *cstr) {
    this->format<char const *>(cstr);
  }

  template <> inline void format<char>(char ch) {
    zx_u32 const capacity_left = zx_chk_sub_u32(this->capacity, this->offset);
    if (capacity_left > 0) {
      this->buf_[this->offset] = ch;
      this->offset += 1;
    }
  }

  template <> inline void format<zx_u64>(zx_u64 val) {
    constexpr zx_u32 MAX_NUM_DIGITS = 20;
    char digits[MAX_NUM_DIGITS];

    zx_u32 pos = MAX_NUM_DIGITS;
    do {
      pos--;
      digits[pos] = '0' + (char)(val % 10);
      val /= 10;
    } while (val > 0);
    zx_u32 const len = MAX_NUM_DIGITS - pos;

    this->format<ZXFmtStr>(ZXFmtStr{.str = &digits[pos], .len = len});
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
      zx_u32 const capacity_left = zx_chk_sub_u32(this->capacity, this->offset);
      if (capacity_left > 0) {
        this->buf_[this->offset] = '-';
        this->offset += 1;
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

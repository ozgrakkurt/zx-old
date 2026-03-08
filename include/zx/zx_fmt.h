#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_def.h>
#include <zx/zx_int.h>

typedef struct ZXFmtStr {
  char const *str;
  zx_u32 len;
} ZXFmtStr;

typedef struct ZXFmt {
  char *buf;
  zx_u32 capacity;
  zx_u32 offset;
} ZXFmtBuf;

static inline ZXFmt ZXFmt_init(char *buf, zx_u32 capacity) {
  return (ZXFmtBuf){
      .buf = buf,
      .capacity = capacity,
      .offset = 0,
  };
}

static inline zx_u32 ZXFmt_n_written(ZXFmt const *fmt) { return fmt->offset; }

static inline void ZXFmt_reset(ZXFmt *fmt) { fmt->offset = 0; }

static inline void ZXFmt_format_str(ZXFmt *fmt, ZXFmtStr str) {
  zx_u32 const capacity_left = fmt->capacity - fmt->offset;
  zx_usize const n_write = zx_int_min_usize(capacity_left, str->len);
  __builtin_memcpy(&fmt->buf[fmt->offset], str->str, n_write);
  fmt->offset += n_write;
}

static inline void ZXFmt_format_cstr(ZXFmt *fmt, char const *cstr) {
  ZXFmt_format_str(fmt, (ZXFmtStr){.str = cstr, .len = __builtin_strlen(cstr)});
}

static inline void ZXFmt_format_u64(ZXFmt *fmt, zx_u64 val) {
  constexpr zx_u32 MAX_NUM_DIGITS = 20;
  char digits[MAX_NUM_DIGITS];

  zx_u32 pos = MAX_NUM_DIGITS;
  do {
    pos--;
    digits[pos] = '0' + (char)(val % 10);
    val /= 10;
  } while (val > 0);
  zx_u32 const len = MAX_NUM_DIGITS - pos;

  ZXFmt_format_str(fmt, (ZXFmtStr){.str = digits, .len = len});
}

static inline void ZXFmt_format_i64(ZXFmt *fmt, zx_i64 val) {
  if (val >= 0) {
    ZXFmt_format_u64(fmt, (zx_u64)val);
  } else {
    zx_u32 const capacity_left = fmt->capacity - fmt->offset;
    if (capacity_left > 0) {
      fmt->buf[fmt->offset] = '-';
      fmt->offset += 1;
      ZXFmt_format_u64(fmt, (zx_u64)-val);
    }
  }
}

static inline void ZXFmt_format_ch(ZXFmt *fmt, char ch) {
  zx_u32 const capacity_left = fmt->capacity - fmt->offset;
  if (capacity_left > 0) {
    fmt->buf[fmt->offset] = ch;
    fmt->offset += 1;
  }
}

#define ZX_FMT_FORMAT_G_(fmt, v)                                               \
  _Generic((v),                                                                \
      zx_u8: ZXFmt_format_u64,                                                 \
      zx_u16: ZXFmt_format_u64,                                                \
      zx_u32: ZXFmt_format_u64,                                                \
      zx_u64: ZXFmt_format_u64,                                                \
      zx_i8: ZXFmt_format_i64,                                                 \
      zx_i16: ZXFmt_format_i64,                                                \
      zx_i32: ZXFmt_format_i64,                                                \
      zx_i64: ZXFmt_format_i64,                                                \
      char: ZXFmt_format_ch,                                                   \
      const char *: ZXFmt_format_cstr,                                         \
      ZXFmtStr: ZXFmt_format_str)((fmt), (v))

#define ZX_FMT_1_(fmt, v) ZX_FMT_FORMAT_G_(fmt, v)
#define ZX_FMT_2_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_1_(fmt, __VA_ARGS__))
#define ZX_FMT_3_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_2_(fmt, __VA_ARGS__))
#define ZX_FMT_4_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_3_(fmt, __VA_ARGS__))
#define ZX_FMT_5_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_4_(fmt, __VA_ARGS__))
#define ZX_FMT_6_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_5_(fmt, __VA_ARGS__))
#define ZX_FMT_7_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_6_(fmt, __VA_ARGS__))
#define ZX_FMT_8_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_7_(fmt, __VA_ARGS__))
#define ZX_FMT_9_(fmt, v, ...)                                                 \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_8_(fmt, __VA_ARGS__))
#define ZX_FMT_10_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_9_(fmt, __VA_ARGS__))
#define ZX_FMT_11_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_10_(fmt, __VA_ARGS__))
#define ZX_FMT_12_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_11_(fmt, __VA_ARGS__))
#define ZX_FMT_13_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_12_(fmt, __VA_ARGS__))
#define ZX_FMT_14_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_13_(fmt, __VA_ARGS__))
#define ZX_FMT_15_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_14_(fmt, __VA_ARGS__))
#define ZX_FMT_16_(fmt, v, ...)                                                \
  ZX_FMT_FORMAT_G_(fmt, v) __VA_OPT__(; ZX_FMT_15_(fmt, __VA_ARGS__))
#define ZX_FMT(fmt, v, ...) ZX_FMT_16_(fmt, v, __VA_ARGS__)

#endif

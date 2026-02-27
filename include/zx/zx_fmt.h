#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

static inline ZXStatus zx_fmt_write_u64(char *buf, zx_usize capacity,
                                        zx_usize *offset, zx_u64 val,
                                        ZXErrStack *err_stack) {
  constexpr zx_usize MAX_NUM_DIGITS = 22;
  char digits[MAX_NUM_DIGITS];

  zx_usize pos = MAX_NUM_DIGITS;
  while (val > 0) {
    pos--;
    digits[pos] = val % 10;
    val /= 10;
  }
  zx_usize const len = MAX_NUM_DIGITS - pos;

  zx_usize new_offset = zx_chk_add(*offset, len);
  ZX_ERR_ASSERT((new_offset < capacity), err_stack);

  __builtin_memcpy(buf, &digits[pos], len);
  buf[new_offset] = 0;
  *offset = new_offset;

  return ZXStatus_Ok;
}

static inline ZXStatus zx_fmt_write_str(char *buf, zx_usize capacity,
                                        zx_usize *offset, char const *str,
                                        zx_usize len, ZXErrStack *err_stack) {
  zx_usize new_offset = zx_chk_add(*offset, len);
  ZX_ERR_ASSERT((new_offset < capacity), err_stack);

  __builtin_memcpy(buf, str, len);
  buf[new_offset] = 0;
  *offset = new_offset;

  return ZXStatus_Ok;
}

typedef struct ZXFmt {
  char *buf;
  zx_usize capacity;
  zx_usize offset;
} ZXFmt;

static inline ZXFmt ZXFmt_init(char *buf, zx_usize capacity) {
  return (ZXFmt){
      .buf = buf,
      .capacity = capacity,
      .offset = 0,
  };
}

static inline ZXStatus ZXFmt_write_u64(ZXFmt *fmt, zx_u64 val,
                                       ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      zx_fmt_write_u64(fmt->buf, fmt->capacity, &fmt->offset, val, err_stack),
      err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus ZXFmt_write_str(ZXFmt *fmt, char const *str,
                                       zx_usize len, ZXErrStack *err_stack) {
  ZX_ERR_TRY(zx_fmt_write_str(fmt->buf, fmt->capacity, &fmt->offset, str, len,
                              err_stack),
             err_stack);
  return ZXStatus_Ok;
}

#endif

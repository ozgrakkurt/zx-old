#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_sys.h>

static inline ZXStatus zx_fmt_format_u64(char *buf, zx_usize capacity,
                                         zx_usize *offset, zx_u64 val,
                                         ZXErrStack *err_stack) {
  constexpr zx_usize MAX_NUM_DIGITS = 22;
  char digits[MAX_NUM_DIGITS];

  zx_usize pos = MAX_NUM_DIGITS;
  do {
    pos--;
    digits[pos] = '0' + (char)(val % 10);
    val /= 10;
  } while (val > 0);
  zx_usize const len = MAX_NUM_DIGITS - pos;

  zx_usize const off = *offset;
  zx_usize new_offset = zx_chk_add_usize(off, len);
  ZX_ERR_ASSERT((new_offset < capacity), err_stack);

  __builtin_memcpy(&buf[off], &digits[pos], len);
  buf[new_offset] = 0;
  *offset = new_offset;

  return ZXStatus_Ok;
}

static inline ZXStatus zx_fmt_format_i64(char *buf, zx_usize capacity,
                                         zx_usize *offset, zx_i64 val,
                                         ZXErrStack *err_stack) {
  if (val >= 0) {
    ZX_ERR_TRY(
        (zx_fmt_format_u64(buf, capacity, offset, (zx_u64)val, err_stack)),
        err_stack);
  } else {
    zx_usize const off = *offset;
    zx_usize const new_offset = zx_chk_add_usize(off, 1);
    ZX_ERR_ASSERT((new_offset < capacity), err_stack);

    buf[off] = '-';
    buf[new_offset] = 0;
    *offset = new_offset;

    ZX_ERR_TRY(
        (zx_fmt_format_u64(buf, capacity, offset, (zx_u64)-val, err_stack)),
        err_stack);
  }

  return ZXStatus_Ok;
}

static inline ZXStatus zx_fmt_format_str(char *buf, zx_usize capacity,
                                         zx_usize *offset, char const *str,
                                         zx_usize len, ZXErrStack *err_stack) {
  zx_usize const off = *offset;
  zx_usize new_offset = zx_chk_add_usize(off, len);
  ZX_ERR_ASSERT((new_offset < capacity), err_stack);

  __builtin_memcpy(&buf[off], str, len);
  buf[new_offset] = 0;
  *offset = new_offset;

  return ZXStatus_Ok;
}

static inline ZXStatus zx_fmt_format_cstr(char *buf, zx_usize capacity,
                                          zx_usize *offset, char const *str,
                                          ZXErrStack *err_stack) {
  ZX_ERR_TRY((zx_fmt_format_str(buf, capacity, offset, str,
                                __builtin_strlen(str), err_stack)),
             err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus zx_fmt_format_char(char *buf, zx_usize capacity,
                                          zx_usize *offset, char ch,
                                          ZXErrStack *err_stack) {
  zx_usize const off = *offset;
  zx_usize new_offset = zx_chk_add_usize(off, 1);
  ZX_ERR_ASSERT((new_offset < capacity), err_stack);

  buf[off] = ch;
  buf[new_offset] = 0;
  *offset = new_offset;

  return ZXStatus_Ok;
}

typedef struct ZXFmtBuf {
  char *buf;
  zx_usize capacity;
  zx_usize offset;
} ZXFmtBuf;

static inline ZXFmtBuf ZXFmtBuf_init(char *buf, zx_usize capacity) {
  return (ZXFmtBuf){
      .buf = buf,
      .capacity = capacity,
      .offset = 0,
  };
}

static inline ZXStatus ZXFmtBuf_write(ZXFmtBuf const *fmt, zx_i32 fd,
                                      ZXErrStack *err_stack) {
  ZX_ASSERT((fmt->offset < fmt->capacity));
  ZX_ASSERT((fmt->buf[fmt->offset] == 0));

  zx_usize n_written = 0;
  zx_usize const status = zx_sys_write(fd, fmt->buf, fmt->offset, &n_written);

  if (status != ZX_SYS_STATUS_SUCCESS) {
    ZX_ERR_RETURN((zx_err_convert_sys_status(status)), err_stack);
  }

  if (n_written < fmt->offset) {
    ZX_ERR_RETURN((ZXStatus_ShortWrite), err_stack);
  } else if (n_written > fmt->offset) {
    ZX_ERR_RETURN((ZXStatus_LongWrite), err_stack);
  } else {
    return ZXStatus_Ok;
  }
}

static inline ZXStatus ZXFmtBuf_format_u64(ZXFmtBuf *fmt, zx_u64 val,
                                           ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      zx_fmt_format_u64(fmt->buf, fmt->capacity, &fmt->offset, val, err_stack),
      err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus ZXFmtBuf_format_i64(ZXFmtBuf *fmt, zx_i64 val,
                                           ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      zx_fmt_format_i64(fmt->buf, fmt->capacity, &fmt->offset, val, err_stack),
      err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus ZXFmtBuf_format_str(ZXFmtBuf *fmt, char const *str,
                                           zx_usize len,
                                           ZXErrStack *err_stack) {
  ZX_ERR_TRY(zx_fmt_format_str(fmt->buf, fmt->capacity, &fmt->offset, str, len,
                               err_stack),
             err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus ZXFmtBuf_format_cstr(ZXFmtBuf *fmt, char const *str,
                                            ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      zx_fmt_format_cstr(fmt->buf, fmt->capacity, &fmt->offset, str, err_stack),
      err_stack);
  return ZXStatus_Ok;
}

static inline ZXStatus ZXFmtBuf_format_char(ZXFmtBuf *fmt, char ch,
                                            ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      zx_fmt_format_char(fmt->buf, fmt->capacity, &fmt->offset, ch, err_stack),
      err_stack);
  return ZXStatus_Ok;
}

#endif

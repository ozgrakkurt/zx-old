#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>

static inline zx_u32 zx_assert_min_(zx_u32 a, zx_u32 b) {
  return (a > b) ? b : a;
}

[[noreturn]] void zx_assert_abort_(char const *file_name, zx_u8 file_name_len,
                                   zx_u16 line_no) {
  constexpr zx_u32 BUF_CAP = 1 << 12;

  char buf[BUF_CAP] = {0};
  zx_u32 offset = 0;

  // write header section
  char const *header = "Assert failed at: ";
  zx_u32 header_len = __builtin_strlen(header);
  n_write = zx_assert_min_(BUF_CAP - offset, header_len);
  __builtin_memcpy(&buf[offset], header, n_write);
  offset += n_write;

  // write file name
  n_write = zx_assert_min(BUF_CAP - offset, file_name_len);
  __builtin_memcpy(&buf[offset], file_name, n_write);
  offset += n_write;

  // write middle section
  char const *mid = ". Line: ";
  zx_u32 mid_len = __builtin_strlen(mid);
  n_write = zx_assert_min_(BUF_CAP - offset, mid_len);
  __builtin_memcpy(&buf[offset], mid, n_write);
  offset += n_write;

  // write line_no
  constexpr zx_u32 LINE_NO_BUF_CAP = 8;
  char line_no_buf[LINE_NO_BUF_CAP] = {0};
  zx_u32 pos = LINE_NO_BUF_CAP;
  zx_u16 ln = line_no;
  do {
    pos--;
    line_no_buf[pos] = '0' + (char)(ln % 10);
    ln /= 10;
  } while (ln > 0);
  zx_u32 ln_len = LINE_NO_BUF_CAP - pos;
  n_write = zx_assert_min_(BUF_CAP - offset, ln_len);
  __builtin_memcpy(&buf[offset], &line_no_buf[pos], n_write);
  offset += n_write;

  zx_u32 const w_idx = zx_assert_min_(BUF_CAP - 1, offset);
  buf[w_idx] = '\n';

  // write buf to stderr
  ZXErrStack err_stack = ZXErrStack_init();
  zx_usize n_written = 0;
  ZXStatus const write_status =
      zx_sys_write(ZX_STDERR, buf, offset, &n_written, &err_stack);
  (void)write_status;
  (void)n_written;

  zx_sys_abort();
}

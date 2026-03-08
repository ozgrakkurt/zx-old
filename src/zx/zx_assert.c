#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

static inline zx_u32 zx_assert_min_(zx_u32 a, zx_u32 b) {
  return (a > b) ? b : a;
}

[[noreturn]] void zx_assert_abort_(char const *file_name, zx_u8 file_name_len,
                                   zx_u16 line_no) {
  constexpr zx_u32 BUF_CAP = 1 << 12;
  char buf[BUF_CAP] = {0};
  ZXFmt fmt = ZXFmt_init(buf, BUF_CAP);

  ZX_FMT(&fmt, "Assert failed at: ", file_name, "[line=", line_no, "]\n");

  // write buf to stderr
  ZXErrStack err_stack = ZXErrStack_init();
  zx_u64 n_written = 0;
  ZXStatus const write_status = zx_sys_write(
      ZX_STDERR, buf, ZXFmt_n_written(&fmt), &n_written, &err_stack);
  (void)write_status;
  (void)n_written;

  zx_sys_abort();
}

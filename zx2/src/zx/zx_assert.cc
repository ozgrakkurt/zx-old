#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

[[noreturn]] void zx_assert_abort_(char const *file_name, zx_u8 file_name_len,
                                   zx_u16 line_no) {
  constexpr zx_u32 BUF_CAP = 1 << 12;
  char buf[BUF_CAP] = {0};
  ZXFmt fmt = ZXFmt::init(buf, BUF_CAP);

  ZXFmtStr const file_name_s = {.str = file_name, .len = file_name_len};

  ZX_FMT(&fmt, "Assert failed at: ", file_name_s, "[line=", line_no, "]\n");

  // write buf to stderr
  ZXErrStack err_stack = ZXErrStack::init();
  zx_u64 n_written = 0;
  ZXStatus const write_status =
      zx_sys_write(ZX_STDERR, (zx_u8 const *)fmt.buf(), fmt.n_written(),
                   &n_written, &err_stack);
  (void)write_status;
  (void)n_written;

  zx_sys_abort();
}

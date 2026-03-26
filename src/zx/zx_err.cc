#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

static void zx_err_format_(ZXFmt *fmt, ZXErrStack const *err_stack) {
  ZX_FMT(fmt, "Error trace: \n");

  for (zx_u64 i = 0; i < err_stack->depth; i++) {
    ZXErrStackEntry_ const entry = err_stack->entries[i];
    ZXFmtStr file_name = {.str = entry.file_name, .len = entry.file_name_len};
    ZX_FMT(fmt, file_name, "[line=", entry.line_no, "]\n");
  }
}

[[noreturn]] void ZXErrStack::abort_() const {
  constexpr zx_u64 BUF_CAP = 1 << 12;
  char buf[BUF_CAP] = {0};
  ZXFmt fmt = ZXFmt::init(buf, BUF_CAP);

  ZXErrStack err_s = ZXErrStack::init();

  zx_err_format_(&fmt, this);

  zx_u64 n_written = 0;
  ZXStatus const status = zx_sys_write(ZX_STDERR, (zx_u8 const *)fmt.buf(),
                                       fmt.n_written(), &n_written, &err_s);
  (void)status;
  (void)n_written;

  zx_sys_abort();
}

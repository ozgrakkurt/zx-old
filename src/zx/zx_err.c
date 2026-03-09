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

[[noreturn]] void zx_err_abort_(ZXErrStack const *err_stack) {
  constexpr zx_u64 BUF_CAP = 1 << 12;
  char buf[BUF_CAP] = {0};
  ZXFmt fmt = ZXFmt_init(buf, BUF_CAP);

  ZXErrStack err_s = ZXErrStack_init();

  zx_err_format_(&fmt, err_stack);

  zx_u64 n_written = 0;
  zx_u64 const status =
      zx_sys_write(ZX_STDERR, buf, ZXFmt_n_written(&fmt), &n_written, &err_s);
  (void)status;
  (void)n_written;

  zx_sys_abort();
}

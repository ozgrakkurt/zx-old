#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

ZXStatus zx_fuzz_setup(void **usr_ctx, ZXErrStack *err_stack);
ZXStatus zx_fuzz_one(void *usr_ctx, zx_u8 const *fuzz_input,
                     zx_usize fuzz_input_len, zx_usize *n_consumed,
                     ZXErrStack *err_stack);

extern bool __start_zx_fuzz_marker_is_hit;
extern int const __start_zx_fuzz_marker_lineno;

extern char const __start_zx_fuzz_marker_filename;
extern char const __stop_zx_fuzz_marker_filename;

extern bool __start_zx_fuzz_err_is_hit;
extern int const __start_zx_fuzz_err_lineno;

extern char const __start_zx_fuzz_err_filename;
extern char const __stop_zx_fuzz_err_filename;

extern int const __start_zx_fuzz_abort_lineno;
extern char const __start_zx_fuzz_abort_filename;
extern char const __stop_zx_fuzz_abort_filename;

constexpr zx_usize MEM_CAP = 1 << 30;
constexpr zx_usize FMT_BUF_CAP = 1 << 14;

static ZXStatus print_counts(ZXFmtBuf *fmt_buf, zx_usize n_markers,
                             zx_usize n_errs, zx_usize n_aborts,
                             ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "Found:\n\t MARKERS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, n_markers, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ERRS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, n_errs, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ABORTS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, n_aborts, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n", err_stack)), err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(fmt_buf, ZX_STDERR, err_stack)), err_stack);

  ZXFmtBuf_reset(fmt_buf);

  return ZXStatus_Ok;
}

static void clear_is_hit(zx_usize n_markers, zx_usize n_errs) {
  __builtin_memset(&__start_zx_fuzz_marker_is_hit, false, n_markers);
  __builtin_memset(&__start_zx_fuzz_err_is_hit, false, n_errs);
}

static void count_is_hit(zx_usize n_markers, zx_usize n_errs,
                         zx_usize *n_markers_hit, zx_usize *n_errs_hit) {
  zx_usize marker_count = 0;
  for (zx_usize i = 0; i < n_markers; i++) {
    if ((&__start_zx_fuzz_marker_is_hit)[i]) {
      marker_count++;
    }
  }
  zx_usize err_count = 0;
  for (zx_usize i = 0; i < n_errs; i++) {
    if ((&__start_zx_fuzz_err_is_hit)[i]) {
      err_count++;
    }
  }

  *n_markers_hit = marker_count;
  *n_errs_hit = err_count;
}

static ZXStatus print_hit_counts(ZXFmtBuf *fmt_buf, zx_usize n_markers_hit,
                                 zx_usize n_errs_hit, ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "Hit:\n\t MARKERS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, n_markers_hit, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ERRS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, n_errs_hit, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n", err_stack)), err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(fmt_buf, ZX_STDERR, err_stack)), err_stack);

  ZXFmtBuf_reset(fmt_buf);

  return ZXStatus_Ok;
}

int main(void) {
  zx_usize const n_markers = ((zx_usize)&__stop_zx_fuzz_marker_filename) -
                             ((zx_usize)&__start_zx_fuzz_marker_filename);
  zx_usize const n_errs = ((zx_usize)&__stop_zx_fuzz_err_filename) -
                          ((zx_usize)&__start_zx_fuzz_err_filename);
  zx_usize const n_aborts = ((zx_usize)&__stop_zx_fuzz_abort_filename) -
                            ((zx_usize)&__start_zx_fuzz_abort_filename);

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  char *fmt_buf_alloc = nullptr;
  ZX_ERR_TRACE_ABORT(
      (ZXAlloc_alloc(&alloc, FMT_BUF_CAP, (void **)&fmt_buf_alloc, &err_stack)),
      &err_stack);
  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_alloc, FMT_BUF_CAP);

  ZX_ERR_TRACE_ABORT(
      (print_counts(&fmt_buf, n_markers, n_errs, n_aborts, &err_stack)),
      &err_stack);

  void *usr_ctx = nullptr;
  ZX_ERR_TRACE_ABORT((zx_fuzz_setup(&usr_ctx, &err_stack)), &err_stack);

  while (true) {
    clear_is_hit(n_markers, n_errs);
    zx_usize n_consumed = 0;
    ZX_ERR_TRACE_ABORT(
        (zx_fuzz_one(usr_ctx, (zx_u8 const *)"anan", __builtin_strlen("anan"),
                     &n_consumed, &err_stack)),
        &err_stack);
    ZX_FUZZ_MARKER;

    zx_usize n_marker_hit = 0;
    zx_usize n_err_hit = 0;
    count_is_hit(n_markers, n_errs, &n_marker_hit, &n_err_hit);
    ZX_ERR_TRACE_ABORT(
        (print_hit_counts(&fmt_buf, n_marker_hit, n_err_hit, &err_stack)),
        &err_stack);
  }

  zx_page_alloc_free(mem, MEM_CAP);
  return 0;
}

#endif

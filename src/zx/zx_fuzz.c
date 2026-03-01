#ifdef ZX_FUZZING_ENABLED

#include "./zx_fuzz_linker_set.h"
#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

static ZXStatus zx_fuzz_print_counts(ZXFuzzLinkerSet const *linker_set,
                                     ZXFmtBuf *fmt_buf, ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "Found:\n\t MARKERS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, linker_set->n_markers, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ERRS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, linker_set->n_errs, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ABORTS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, linker_set->n_aborts, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\n", err_stack)), err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(fmt_buf, ZX_STDERR, err_stack)), err_stack);

  ZXFmtBuf_reset(fmt_buf);

  return ZXStatus_Ok;
}

static void zx_fuzz_clear_is_hit(ZXFuzzLinkerSet const *linker_set) {
  __builtin_memset(linker_set->marker_is_hit, false, linker_set->n_markers);
  __builtin_memset(linker_set->err_is_hit, false, linker_set->n_errs);
}

static zx_usize zx_fuzz_count_is_hit(bool const *is_hit, zx_usize is_hit_len) {
  zx_usize count = 0;
  for (zx_usize i = 0; i < is_hit_len; i++) {
    if (is_hit[i]) {
      count++;
    }
  }

  return count;
}

static ZXStatus zx_fuzz_print_hit_counts(ZXFmtBuf *fmt_buf,
                                         zx_usize n_markers_hit,
                                         zx_usize n_errs_hit,
                                         ZXErrStack *err_stack) {
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
  constexpr zx_usize MEM_CAP = 1 << 30;
  constexpr zx_usize FMT_BUF_CAP = 1 << 14;

  ZXFuzzLinkerSet const linker_set = ZXFuzzLinkerSet_load();

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  char *fmt_buf_alloc = nullptr;
  ZX_ERR_TRACE_ABORT(
      (ZXAlloc_alloc(&alloc, FMT_BUF_CAP, (void **)&fmt_buf_alloc, &err_stack)),
      &err_stack);
  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_alloc, FMT_BUF_CAP);

  ZX_ERR_TRACE_ABORT((zx_fuzz_print_counts(&linker_set, &fmt_buf, &err_stack)),
                     &err_stack);

  void *usr_ctx = nullptr;
  ZX_ERR_TRACE_ABORT((zx_fuzz_setup(&usr_ctx, &err_stack)), &err_stack);

  while (true) {
    zx_fuzz_clear_is_hit(&linker_set);
    zx_usize n_consumed = 0;
    ZX_ERR_TRACE_ABORT(
        (zx_fuzz_one(usr_ctx, (zx_u8 const *)"anan", __builtin_strlen("anan"),
                     &n_consumed, &err_stack)),
        &err_stack);
    ZX_FUZZ_MARKER;

    zx_usize const n_marker_hit =
        zx_fuzz_count_is_hit(linker_set.marker_is_hit, linker_set.n_markers);
    zx_usize const n_err_hit =
        zx_fuzz_count_is_hit(linker_set.err_is_hit, linker_set.n_errs);
    ZX_ERR_TRACE_ABORT((zx_fuzz_print_hit_counts(&fmt_buf, n_marker_hit,
                                                 n_err_hit, &err_stack)),
                       &err_stack);
  }

  zx_page_alloc_free(mem, MEM_CAP);
  return 0;
}

#endif

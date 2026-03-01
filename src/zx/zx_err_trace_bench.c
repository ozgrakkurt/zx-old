#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>

static inline ZXStatus err_func(zx_usize depth, ZXErrStack *err_stack) {
  if (depth > 128) {
    ZX_ERR_RETURN((ZXStatus_UnexpectedError), err_stack);
  }
  if (depth > 64) {
    ZX_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else if (depth > 32) {
    ZX_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else if (depth > 16) {
    ZX_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else {
    ZX_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  }
  return ZXStatus_Ok;
}

static inline ZXStatus print_timing(ZXAlloc *alloc, const char *head,
                                    zx_u64 timing, const char *tail,
                                    ZXErrStack *err_stack) {
  constexpr zx_usize fmt_buf_size = 1 << 12;
  char *fmt_buf_alloc = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, fmt_buf_size, (void **)&fmt_buf_alloc, err_stack)),
      err_stack);

  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_alloc, fmt_buf_size);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, head, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, timing, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, tail, err_stack)), err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(&fmt_buf, ZX_STDERR, err_stack)), err_stack);

  return ZXStatus_Ok;
}

static inline ZXStatus recurse_func(ZXAlloc *alloc, ZXErrStack *err_stack) {
  (void)alloc;

  ZXTimer timer = ZXTimer_init();
  ZXStatus const status = err_func(1, err_stack);
  zx_u64 const elapsed = ZXTimer_lap(&timer);

  ZX_ERR_TRY((print_timing(alloc, "recurse_func elapsed: ", elapsed, " ticks\n",
                           err_stack)),
             err_stack);

  ZX_ERR_TRY(status, err_stack);

  return ZXStatus_Ok;
}

static inline ZXStatus do_bench(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXStatus const status = recurse_func(alloc, err_stack);

  if (status != ZXStatus_Ok) {
    char const *trace = nullptr;
    zx_usize trace_size = 0;

    // secondary error stack for tracking errors happening while collecting the
    // error trace
    ZXErrStack collect_stack = ZXErrStack_init();
    ZXTimer timer = ZXTimer_init();
    ZX_ERR_TRY((zx_err_trace_collect(alloc, err_stack, &trace, &trace_size,
                                     &collect_stack)),
               &collect_stack);
    zx_u64 const elapsed = ZXTimer_lap(&timer);

    ZX_ERR_TRY((print_timing(alloc, "collect func elapsed: ", elapsed,
                             " ticks\n", err_stack)),
               err_stack);

    // zx_usize n_written = 0;
    // zx_sys_write(ZX_STDERR, trace, trace_size, &n_written);
    // ZX_ASSERT((n_written == trace_size));
  }

  // ZX_DBG_ASSERT(false);
  // ZX_ASSERT(false);
  // zx_page_alloc_free(nullptr, MEMORY_SIZE);
  //

  return ZXStatus_Ok;
}

constexpr zx_usize MEMORY_SIZE = 1 << 16;

int main(void) {
  void *const page = zx_page_alloc_4kb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  for (zx_usize i = 0; i < 50; i++) {
    ZXAlloc_reset(&alloc);
    ZX_ERR_TRACE_TRY_ABORT((do_bench(&alloc, &err_stack)), &err_stack);
  }

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

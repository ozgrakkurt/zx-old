#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

ZXStatus err_func(zx_usize depth, ZXErrStack *err_stack) {
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

ZXStatus test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  (void)alloc;

  ZX_ERR_TRY((err_func(1, err_stack)), err_stack);

  return ZXStatus_Ok;
}

constexpr zx_usize MEMORY_SIZE = 1 << 16;

int main(void) {
  void *const page = zx_page_alloc_4kb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  ZXStatus const status = test(&alloc, &err_stack);

  if (status != ZXStatus_Ok) {
    char const *trace = nullptr;
    zx_usize trace_size = 0;

    // secondary error stack for tracking errors happening while collecting the
    // error trace
    ZXErrStack collect_stack = ZXErrStack_init();
    ZX_ERR_TRACE_TRY_ABORT((zx_err_trace_collect(&alloc, &err_stack, &trace,
                                                 &trace_size, &collect_stack)),
                           &collect_stack);

    // zx_usize n_written = 0;
    // zx_sys_write(ZX_STDERR, trace, trace_size, &n_written);
    // ZX_ASSERT((n_written == trace_size));
  }

  // ZX_DBG_ASSERT(false);
  // ZX_ASSERT(false);
  // zx_page_alloc_free(nullptr, MEMORY_SIZE);

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

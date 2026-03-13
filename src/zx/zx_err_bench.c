#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>

static inline ZXStatus err_func(zx_u32 depth, ZXErrStack *err_stack) {
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

static inline ZXStatus recurse_func(ZXAlloc *alloc, ZXFmt *fmt,
                                    ZXErrStack *err_stack) {
  (void)alloc;

  ZXTimer timer = ZXTimer_init();
  ZXStatus const status = err_func(1, err_stack);
  zx_u64 const elapsed = ZXTimer_lap(&timer);

  ZX_FMT(fmt, "recurse_func elapsed: ", elapsed, " ticks\n");

  ZX_ERR_TRY(status, err_stack);

  return ZXStatus_Ok;
}

static inline ZXStatus do_bench(ZXAlloc *alloc, ZXFmt *fmt,
                                ZXErrStack *err_stack) {
  ZXStatus const status = recurse_func(alloc, fmt, err_stack);
  // ZX_ERR_TRY((status), err_stack);
  (void)status;

  // ZX_DBG_ASSERT(false);
  // ZX_ASSERT(false);
  // zx_page_alloc_free(nullptr, MEMORY_SIZE);
  //

  return ZXStatus_Ok;
}

constexpr zx_u32 MEMORY_SIZE = 1 << 16;
constexpr zx_u32 FMT_BUF_SIZE = 1 << 12;

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  char fmt_buf[FMT_BUF_SIZE] = {0};
  ZXFmt fmt = ZXFmt_init(fmt_buf, FMT_BUF_SIZE);

  for (zx_u32 i = 0; i < 50; i++) {
    ZXAlloc_reset(&alloc);
    ZXFmt_reset(&fmt);
    ZX_ERR_TRY_FATAL((do_bench(&alloc, &fmt, &err_stack)), &err_stack);
  }

  zx_u64 const to_write = ZXFmt_n_written(&fmt);
  zx_u64 n_written = 0;
  ZX_ERR_TRY_FATAL((zx_sys_write(ZX_STDERR, (zx_u8 const *)fmt_buf, to_write,
                                 &n_written, &err_stack)),
                   &err_stack);
  ZX_ASSERT((n_written == to_write));

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

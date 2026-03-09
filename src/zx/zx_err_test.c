#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

ZXStatus err_func(zx_u64 depth, ZXErrStack *err_stack) {
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

constexpr zx_u64 MEMORY_SIZE = 1 << 16;

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  void *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZXStatus const status = test(&alloc, &err_stack);
  // ZX_ERR_TRY_FATAL((status), &err_stack);
  (void)status;

  // ZX_DBG_ASSERT(false);
  // ZX_ASSERT(false);
  // zx_page_alloc_free(nullptr, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

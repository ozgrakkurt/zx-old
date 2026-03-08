#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_page_alloc.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 16;

ZXStatus test_allocator(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXAlloc_reset(alloc);

  zx_i32 **slice = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, 100 * sizeof(zx_i32 *), (void **)&slice,
                            err_stack)),
             err_stack);
  ZX_ERR_ASSERT((slice != nullptr), err_stack);
  ZX_ERR_ASSERT((((zx_u64)slice) % alignof(zx_i32 *) == 0), err_stack);

  for (zx_u64 i = 0; i < 100; i++) {
    slice[i] = nullptr;
    ZX_ERR_TRY(
        (ZXAlloc_alloc(alloc, sizeof(zx_i32), (void **)&slice[i], err_stack)),
        err_stack);
    ZX_ERR_ASSERT((slice[i] != nullptr), err_stack);
    ZX_ERR_ASSERT((((zx_u64)slice[i]) % alignof(zx_i32) == 0), err_stack);
    *(slice[i]) = (zx_i32)i;
  }

  for (zx_u64 i = 0; i < 100; i++) {
    ZX_ERR_ASSERT((*(slice[i]) == (zx_i32)i), err_stack);
  }

  zx_u8 *zero_ptr = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, 0, (void **)&zero_ptr, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  zx_u8 *big_ptr = nullptr;
  ZXStatus const oom_status =
      ZXAlloc_alloc(alloc, MEMORY_SIZE, (void **)&big_ptr, err_stack);
  ZX_ERR_ASSERT((oom_status == ZXStatus_OutOfMemory), err_stack);
  ZX_ERR_ASSERT((big_ptr == nullptr), err_stack);

  ZXAlloc_reset(alloc);
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, MEMORY_SIZE, (void **)&big_ptr, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((big_ptr != nullptr), err_stack);

  zero_ptr = nullptr;
  ZXStatus const oom2 = ZXAlloc_alloc(alloc, 1, (void **)&zero_ptr, err_stack);
  ZX_ERR_ASSERT((oom2 == ZXStatus_OutOfMemory), err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  ZX_ERR_TRY((ZXAlloc_alloc(alloc, 0, (void **)&zero_ptr, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  return ZXStatus_Ok;
}

ZXStatus test_allocator_aligned(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXAlloc_reset(alloc);

  zx_u64 const alignments[] = {1, 2, 4, 8, 16, 32, 64};

  zx_u8 *ptr = nullptr;

  for (zx_u64 i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
    zx_u64 const alignment = alignments[i];

    ptr = nullptr;
    ZX_ERR_TRY(
        (ZXAlloc_aligned_alloc(alloc, alignment, 69, (void **)&ptr, err_stack)),
        err_stack);
    ZX_ERR_ASSERT((ptr != nullptr), err_stack);
    ZX_ERR_ASSERT((((zx_u64)ptr) % alignment == 0), err_stack);
    ZX_ERR_ASSERT((((zx_u64)ptr) % alignof(zx_u256) == 0), err_stack);
  }

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_4kb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  ZX_ERR_TRACE_TRY_ABORT((test_allocator(&alloc, &err_stack)), &err_stack);
  ZX_ERR_TRACE_TRY_ABORT((test_allocator_aligned(&alloc, &err_stack)),
                         &err_stack);

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

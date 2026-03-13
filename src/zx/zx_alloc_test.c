#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 16;

ZXStatus test_allocator(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXAlloc_reset(alloc);

  zx_i32 **slice = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_i32), 100 * sizeof(zx_i32 *),
                            (zx_u8 **)&slice, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((slice != nullptr), err_stack);
  ZX_ERR_ASSERT((((zx_u64)slice) % alignof(zx_i32 *) == 0), err_stack);

  for (zx_u64 i = 0; i < 100; i++) {
    slice[i] = nullptr;
    ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_i32), sizeof(zx_i32),
                              (zx_u8 **)&slice[i], err_stack)),
               err_stack);
    ZX_ERR_ASSERT((slice[i] != nullptr), err_stack);
    ZX_ERR_ASSERT((((zx_u64)slice[i]) % alignof(zx_i32) == 0), err_stack);
    *(slice[i]) = (zx_i32)i;
  }

  for (zx_u64 i = 0; i < 100; i++) {
    ZX_ERR_ASSERT((*(slice[i]) == (zx_i32)i), err_stack);
  }

  zx_u8 *zero_ptr = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u8), 0, &zero_ptr, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  zx_u8 *big_ptr = nullptr;
  ZXStatus const oom_status =
      ZXAlloc_alloc(alloc, alignof(zx_u8), MEMORY_SIZE, &big_ptr, err_stack);
  ZX_ERR_ASSERT((oom_status == ZXStatus_OutOfMemory), err_stack);
  ZX_ERR_ASSERT((big_ptr == nullptr), err_stack);

  ZXAlloc_reset(alloc);
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, alignof(zx_u8), MEMORY_SIZE, &big_ptr, err_stack)),
      err_stack);
  ZX_ERR_ASSERT((big_ptr != nullptr), err_stack);

  zero_ptr = nullptr;
  ZXStatus const oom2 =
      ZXAlloc_alloc(alloc, alignof(zx_u8), 1, &zero_ptr, err_stack);
  ZX_ERR_ASSERT((oom2 == ZXStatus_OutOfMemory), err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u8), 0, &zero_ptr, err_stack)),
             err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((test_allocator(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

#include <zx/zx_alloc.h>
#include <zx/zx_debug_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 16;

ZXStatus test_allocator(ZXDebugAlloc *dbg_alloc, ZXErrStack *err_stack) {
  ZXAlloc alloc = dbg_alloc.make_alloc();

  dbg_alloc.reset();

  zx_i32 **slice = nullptr;
  ZX_ERR_TRY((alloc.alloc<zx_i32 *>(100, &slice, err_stack)), err_stack);
  ZX_ERR_ASSERT((slice != nullptr), err_stack);
  ZX_ERR_ASSERT((((zx_u64)slice) % alignof(zx_i32 *) == 0), err_stack);

  for (zx_u64 i = 0; i < 100; i++) {
    slice[i] = nullptr;
    ZX_ERR_TRY((alloc.alloc<zx_i32>(sizeof(zx_i32), &slice[i], err_stack)),
               err_stack);
    ZX_ERR_ASSERT((slice[i] != nullptr), err_stack);
    ZX_ERR_ASSERT((((zx_u64)slice[i]) % alignof(zx_i32) == 0), err_stack);
    *(slice[i]) = (zx_i32)i;
  }

  for (zx_u64 i = 0; i < 100; i++) {
    ZX_ERR_ASSERT((*(slice[i]) == (zx_i32)i), err_stack);
  }

  zx_u8 *zero_ptr = nullptr;
  ZX_ERR_TRY((alloc.alloc<zx_u8>(0, &zero_ptr, err_stack)), err_stack);
  ZX_ERR_ASSERT((zero_ptr != nullptr), err_stack);

  zx_u8 *big_ptr = nullptr;
  ZXStatus const oom_status =
      alloc.alloc<zx_u8>(MEMORY_SIZE, &big_ptr, err_stack);
  ZX_ERR_ASSERT((oom_status == ZXStatus::OutOfMemory), err_stack);
  ZX_ERR_ASSERT((big_ptr == nullptr), err_stack);

  dbg_alloc.reset();
  ZX_ERR_TRY((alloc.alloc<zx_u8>(MEMORY_SIZE, &big_ptr, err_stack)), err_stack);
  ZX_ERR_ASSERT((big_ptr != nullptr), err_stack);

  zero_ptr = nullptr;
  ZXStatus const oom2 = alloc.alloc<zx_u8>(1, &zero_ptr, err_stack);
  ZX_ERR_ASSERT((oom2 == ZXStatus::OutOfMemory), err_stack);
  ZX_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  ZX_ERR_TRY((alloc.alloc(0, &zero_ptr, err_stack)), err_stack);
  ZX_ERR_ASSERT((zero_ptr != nullptr), err_stack);

  return ZXStatus::Ok;
}

// TODO: split this code into debug/arena alloc tests. Build a reusable header
// of test functions that can be used to test any allocator impl.
int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL((ZXDebugAlloc::init(MEMORY_SIZE, &dbg_alloc, &err_stack)),
                   &err_stack);

  ZX_ERR_TRY_FATAL((test_allocator(&dbg_alloc, &err_stack)), &err_stack);

  return 0;
}

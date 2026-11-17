#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_debug_alloc.h>
#include <zx/zx_err.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 12;
constexpr zx_u32 CAP = 3169;

static inline ZXStatus iter_test(ZXAlloc alloc, ZXBitmap const *bmap,
                                 ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap::init(alloc, bmap->n_bits(), &bitmap, err_stack)),
             err_stack);

  ZXBitmapIter iter = bmap->iter();

  zx_u32 set_idx = 0;
  while (iter.next(&set_idx)) {
    bitmap.set(set_idx);
  }

  for (zx_u32 i = 0; i < bmap->n_bits(); i++) {
    ZX_ERR_ASSERT((bmap->get(i) == bitmap.get(i)), err_stack);
  }

  return ZXStatus::Ok;
}

static inline ZXStatus smoke_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap::init(alloc, CAP, &bitmap, err_stack)), err_stack);

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      ZX_ERR_ASSERT((!bitmap.get(i)), err_stack);
      ZX_ERR_ASSERT((!bitmap.set(i)), err_stack);
      ZX_ERR_ASSERT((bitmap.get(i)), err_stack);
    }
  }

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      ZX_ERR_ASSERT((bitmap.get(i)), err_stack);
      ZX_ERR_ASSERT((bitmap.unset(i)), err_stack);
      ZX_ERR_ASSERT((!bitmap.get(i)), err_stack);
    } else {
      ZX_ERR_ASSERT((!bitmap.get(i)), err_stack);
      ZX_ERR_ASSERT((!bitmap.set(i)), err_stack);
      ZX_ERR_ASSERT((bitmap.get(i)), err_stack);
    }
  }

  ZX_ERR_TRY(iter_test(alloc, &bitmap, err_stack), err_stack);

  return ZXStatus::Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap::init(alloc, 0, &bitmap, err_stack)), err_stack);

  ZX_ERR_TRY(iter_test(alloc, &bitmap, err_stack), err_stack);

  return ZXStatus::Ok;
}

static inline ZXStatus reset_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap::init(alloc, CAP, &bitmap, err_stack)), err_stack);

  for (zx_u32 i = 0; i < CAP; i++) {
    bitmap.set(i);
  }

  bitmap.reset();

  for (zx_u32 i = 0; i < CAP; i++) {
    ZX_ERR_ASSERT((!bitmap.get(i)), err_stack);
  }

  return ZXStatus::Ok;
}

static inline ZXStatus boundary_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  // Test with a size that is not a multiple of 64
  zx_u32 n_bits = 70;
  ZX_ERR_TRY((ZXBitmap::init(alloc, n_bits, &bitmap, err_stack)), err_stack);

  for (zx_u32 i = 0; i < n_bits; i++) {
    ZX_ERR_ASSERT((!bitmap.get(i)), err_stack);
  }

  // Set first and last bits
  bitmap.set(0);
  bitmap.set(n_bits - 1);

  ZX_ERR_ASSERT((bitmap.get(0)), err_stack);
  ZX_ERR_ASSERT((bitmap.get(n_bits - 1)), err_stack);
  ZX_ERR_ASSERT((!bitmap.get(1)), err_stack);
  ZX_ERR_ASSERT((!bitmap.get(n_bits - 2)), err_stack);

  // Test iterator with boundary bits
  zx_u32 idx = 0;
  int count = 0;
  ZXBitmapIter iter = bitmap.iter();
  while (iter.next(&idx)) {
    count++;
  }
  ZX_ERR_ASSERT((count == 2), err_stack);
  ZX_ERR_ASSERT((idx == n_bits - 1),
                err_stack); // Last idx should be the last bit set

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL((ZXDebugAlloc::init(MEMORY_SIZE, &dbg_alloc, &err_stack)),
                   &err_stack);
  ZXAlloc alloc = dbg_alloc.make_alloc();

  ZX_ERR_TRY_FATAL((smoke_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((zero_size_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((reset_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((boundary_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  return 0;
}

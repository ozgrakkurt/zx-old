#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_int.h>

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

constexpr zx_u64 MEMORY_SIZE = 1 << 12;
constexpr zx_u32 CAP = 3169;

static void iter_test(ZXAlloc* alloc, ZXBitmap const* bmap) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok ==
         ZXBitmap::init(alloc, bmap->n_bits(), &bitmap));

  ZXBitmapIter iter = bmap->iter();

  zx_u32 set_idx = 0;
  while (ZXBitmapIterStatus::Continue == iter.next(&set_idx)) {
    bitmap.set(set_idx);
  }

  for (zx_u32 i = 0; i < bmap->n_bits(); i++) {
    assert(bmap->get(i) == bitmap.get(i));
  }
}

static void smoke_test(ZXAlloc* alloc) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, CAP, &bitmap));

  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(CAP));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(CAP));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(CAP));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(ZX_U64_MAX));

  for (zx_u32 i = 0; i < CAP; i++) {
    assert(ZXBitmapAccessStatus::Unset == bitmap.get(i));
  }

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      assert(ZXBitmapAccessStatus::NotSet == bitmap.get(i));
      assert(ZXBitmapAccessStatus::NotSet == bitmap.set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap.get(i));
      assert(ZXBitmapAccessStatus::Set == bitmap.set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap.get(i));
    }
  }

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      assert(ZXBitmapAccessStatus::Set == bitmap.get(i));
      assert(ZXBitmapAccessStatus::Set == bitmap.unset(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap.get(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap.unset(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap.get(i));
    } else {
      assert(ZXBitmapAccessStatus::Unset == bitmap.get(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap.set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap.get(i));
    }
  }

  iter_test(alloc, &bitmap);
}

static void zero_size_test(ZXAlloc* alloc) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, 0, &bitmap));
  assert(bitmap.n_bits() == 0);

  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(ZX_U64_MAX));

  iter_test(alloc, &bitmap);
}

static void nocap_test() {}

static void reset_test(ZXAlloc* alloc) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, 0, &bitmap));
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

#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void iter_test(ZXAlloc* alloc, ZXBitmap const* bmap) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok ==
         ZXBitmap::init(alloc, bmap->n_bits(), &bitmap));

  ZXBitmapIter iter = bmap->iter();

  zx_u64 set_idx = 0;
  while (ZXBitmapIterStatus::Continue == iter.next(&set_idx)) {
    assert(ZXBitmapAccessStatus::Unset == bitmap.set(set_idx));
  }

  for (zx_u32 i = 0; i < bmap->n_bits(); i++) {
    assert(bmap->get(i) == bitmap.get(i));
  }
}

static void smoke_test_inner(ZXAlloc* alloc, ZXBitmap* bitmap, zx_u64 len) {
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->get(len));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->set(len));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->unset(len));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->get(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->set(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap->unset(ZX_U64_MAX));

  for (zx_u32 i = 0; i < len; i++) {
    assert(ZXBitmapAccessStatus::Unset == bitmap->get(i));
  }

  for (zx_u32 i = 0; i < len; i++) {
    if (i % 3 == 0) {
      assert(ZXBitmapAccessStatus::Unset == bitmap->get(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap->set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap->get(i));
      assert(ZXBitmapAccessStatus::Set == bitmap->set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap->get(i));
    }
  }

  for (zx_u32 i = 0; i < len; i++) {
    if (i % 3 == 0) {
      assert(ZXBitmapAccessStatus::Set == bitmap->get(i));
      assert(ZXBitmapAccessStatus::Set == bitmap->unset(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap->get(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap->unset(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap->get(i));
    } else {
      assert(ZXBitmapAccessStatus::Unset == bitmap->get(i));
      assert(ZXBitmapAccessStatus::Unset == bitmap->set(i));
      assert(ZXBitmapAccessStatus::Set == bitmap->get(i));
    }
  }

  iter_test(alloc, bitmap);
}

static void smoke_test(ZXAlloc* alloc, zx_u64 len) {
  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, len, &bitmap));

  smoke_test_inner(alloc, &bitmap, len);
  bitmap.reset();
  smoke_test_inner(alloc, &bitmap, len);
}

static void zero_size_test() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());

  ZXBitmap bitmap;
  assert(ZXBitmapInitStatus::OutOfMemory ==
         ZXBitmap::init(&noalloc, 69, &bitmap));
  assert(ZXBitmapInitStatus::OutOfMemory ==
         ZXBitmap::init(&noalloc, 1, &bitmap));

  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(&noalloc, 0, &bitmap));
  assert(bitmap.n_bits() == 0);

  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(0));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.get(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.set(ZX_U64_MAX));
  assert(ZXBitmapAccessStatus::OutOfBounds == bitmap.unset(ZX_U64_MAX));

  iter_test(&noalloc, &bitmap);
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = 1 << 20;

  ZXSliceMut<zx_u8> mem;
  assert(zx_page_alloc_4kb(MEMORY_SIZE, &mem) == ZXPageAllocStatus::Ok);

  zero_size_test();

  ZXAlloc alloc = ZXAlloc::init(mem);

  constexpr zx_u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129,  3169};
  constexpr zx_u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u32 i = 0; i < n_lengths; i++) {
    zx_u32 const len = lengths[i];
    alloc.reset();
    smoke_test(&alloc, len);
  }

  assert(zx_page_free(mem) == ZXPageAllocStatus::Ok);
  return 0;
}

#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 12;
constexpr zx_u32 CAP = 3169;

static inline ZXStatus iter_test(ZXAlloc *alloc, ZXBitmap const *bmap,
                                 ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap_init(alloc, ZXBitmap_n_bits(bmap), &bitmap, err_stack)),
             err_stack);

  ZXBitmapIter iter = ZXBitmap_iter(bmap);

  zx_u32 set_idx = 0;
  while (ZXBitmapIter_next(&iter, &set_idx)) {
    ZXBitmap_set(&bitmap, set_idx);
  }

  for (zx_u32 i = 0; i < ZXBitmap_n_bits(bmap); i++) {
    ZX_ERR_ASSERT((ZXBitmap_get(bmap, i) == ZXBitmap_get(&bitmap, i)),
                  err_stack);
  }

  return ZXStatus_Ok;
}

static inline ZXStatus smoke_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap_init(alloc, CAP, &bitmap, err_stack)), err_stack);

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      ZX_ERR_ASSERT((!ZXBitmap_get(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((!ZXBitmap_set(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((ZXBitmap_get(&bitmap, i)), err_stack);
    }
  }

  for (zx_u32 i = 0; i < CAP; i++) {
    if (i % 3 == 0) {
      ZX_ERR_ASSERT((ZXBitmap_get(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((ZXBitmap_unset(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((!ZXBitmap_get(&bitmap, i)), err_stack);
    } else {
      ZX_ERR_ASSERT((!ZXBitmap_get(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((!ZXBitmap_set(&bitmap, i)), err_stack);
      ZX_ERR_ASSERT((ZXBitmap_get(&bitmap, i)), err_stack);
    }
  }

  ZX_ERR_TRY(iter_test(alloc, &bitmap, err_stack), err_stack);

  return ZXStatus_Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXBitmap bitmap;
  ZX_ERR_TRY((ZXBitmap_init(alloc, 0, &bitmap, err_stack)), err_stack);

  ZX_ERR_TRY(iter_test(alloc, &bitmap, err_stack), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((smoke_test(&alloc, &err_stack)), &err_stack);
  ZXAlloc_reset(&alloc);

  ZX_ERR_TRY_FATAL((zero_size_test(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

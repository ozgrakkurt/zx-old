#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slab.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 val_type;

static inline ZXStatus smoke_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXSlab slab;
  ZX_ERR_TRY((ZXSlab_init(alloc, CAP, alignof(val_type), sizeof(val_type),
                          &slab, err_stack)),
             err_stack);

  zx_u32 *slab_keys = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u32), CAP * sizeof(zx_u32),
                            (zx_u8 **)&slab_keys, err_stack)),
             err_stack);
  __builtin_memset(slab_keys, 0, sizeof(zx_u32) * CAP);

  for (zx_u64 i = 0; i < CAP; i++) {
    // insert and test
    slab_keys[i] = ZXSlab_insert(&slab, &(val_type){i}, sizeof(val_type));
    val_type const *got_val = ZXSlab_get(&slab, slab_keys[i], sizeof(val_type));
    val_type gv = *got_val;
    ZX_ERR_ASSERT((gv == i), err_stack);

    // do backwards test
    got_val = ZXSlab_get(&slab, slab_keys[i / 2], sizeof(val_type));
    gv = *got_val;
    ZX_ERR_ASSERT((gv == (i / 2)), err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    val_type val;
    ZXSlab_remove(&slab, slab_keys[i], sizeof(val_type), &val);
    ZX_ERR_ASSERT((val == (i)), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    val_type const *got_val = ZXSlab_get(&slab, slab_keys[i], sizeof(val_type));
    val_type gv = *got_val;
    ZX_ERR_ASSERT((gv == i), err_stack);
  }

  return ZXStatus_Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXSlab slab;
  ZX_ERR_TRY((ZXSlab_init(alloc, 0, alignof(val_type), sizeof(val_type), &slab,
                          err_stack)),
             err_stack);

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

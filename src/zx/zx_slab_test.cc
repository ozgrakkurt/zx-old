#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slab.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 val_type;

using Slab = ZXSlab<val_type>;

static inline ZXStatus smoke_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  Slab slab;
  ZX_ERR_TRY((Slab::init(alloc, CAP, &slab, err_stack)), err_stack);

  zx_u32 *slab_keys = nullptr;
  ZX_ERR_TRY((alloc->alloc<zx_u32>(CAP, &slab_keys, err_stack)), err_stack);
  __builtin_memset(slab_keys, 0, sizeof(zx_u32) * CAP);

  for (zx_u64 i = 0; i < CAP; i++) {
    // insert and test
    val_type const val = i;
    slab_keys[i] = slab.insert(&val);
    val_type const *got_val = slab.get(slab_keys[i]);
    val_type gv = *got_val;
    ZX_ERR_ASSERT((gv == i), err_stack);

    // do backwards test
    got_val = slab.get(slab_keys[i / 2]);
    gv = *got_val;
    ZX_ERR_ASSERT((gv == (i / 2)), err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    val_type const val = slab.remove(slab_keys[i]);
    ZX_ERR_ASSERT((val == (i)), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    val_type const *got_val = slab.get(slab_keys[i]);
    val_type gv = *got_val;
    ZX_ERR_ASSERT((gv == i), err_stack);
  }

  return ZXStatus::Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  Slab slab;
  ZX_ERR_TRY((Slab::init(alloc, 0, &slab, err_stack)), err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL(());

  ZXAlloc alloc = ZXAlloc::init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((smoke_test(&alloc, &err_stack)), &err_stack);
  alloc.reset();

  ZX_ERR_TRY_FATAL((zero_size_test(&alloc, &err_stack)), &err_stack);

  return 0;
}

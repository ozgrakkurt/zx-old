#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slab.h>
#include <zx/zx_slice.h>

using val_type = zx_u256;
using Slab = ZXSlab<val_type>;

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void smoke_test_inner(Slab *slab, ZXSliceMut<zx_u32> slab_keys,
                             zx_u32 cap) {
  assert(slab->capacity() == cap);
  assert(slab->n_occupied() == 0);

  for (zx_u32 i = 0; i < cap; i++) {
    val_type const val = i;
    zx_u32 idx = ZX_U32_MAX;
    assert(ZXSlabInsertStatus::Ok == slab->insert(&val, &idx));
    slab_keys.write_unsafe(i, idx);
    val_type const *got_val_p = nullptr;
    assert(ZXSlabAccessStatus::Found == slab->get(idx, &got_val_p));
    assert(got_val_p != nullptr);
    assert(*got_val_p == val);

    // backwards test
    val_type *old_val_p = nullptr;
    assert(ZXSlabAccessStatus::Found ==
           slab->get_mut(slab_keys.read_unsafe(i / 2), &old_val_p));
    assert(old_val_p != nullptr);
    assert(*old_val_p == val / 2);
  }

  for (zx_u32 i = 0; i < cap / 2; i++) {
    val_type val;
    assert(ZXSlabAccessStatus::Found ==
           slab->remove(slab_keys.read_unsafe(i), &val));
    assert(val == i);

    val_type const *val_p = nullptr;
    assert(ZXSlabAccessStatus::NotFound ==
           slab->get(slab_keys.read_unsafe(i), &val_p));
    assert(val_p == nullptr);

    val_type *val_p_mut = nullptr;
    assert(ZXSlabAccessStatus::NotFound ==
           slab->get_mut(slab_keys.read_unsafe(i), &val_p_mut));
    assert(val_p_mut == nullptr);

    assert(ZXSlabAccessStatus::NotFound ==
           slab->remove(slab_keys.read_unsafe(i), &val));
  }

  for (zx_u32 i = cap / 2; i < cap; i++) {
    val_type const *val_p = nullptr;
    assert(ZXSlabAccessStatus::Found ==
           slab->get(slab_keys.read_unsafe(i), &val_p));
    assert(val_p != nullptr);
    assert(*val_p == i);
  }
}

static void smoke_test(ZXAlloc *alloc, zx_u32 cap) {
  Slab slab;
  assert(ZXSlabInitStatus::Ok == Slab::init(alloc, cap, &slab));

  ZXSliceMut<zx_u32> slab_keys;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u32>(cap, &slab_keys));

  slab_keys.memset(0xaa);
  smoke_test_inner(&slab, slab_keys, cap);
  slab.reset();
  slab_keys.memset(0xaa);
  smoke_test_inner(&slab, slab_keys, cap);
  slab.reset();
  slab_keys.memset(0xaa);
  smoke_test_inner(&slab, slab_keys, cap);
}

static void noalloc_test() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());
  Slab slab;
  assert(ZXSlabInitStatus::OutOfMemory == Slab::init(&noalloc, 69, &slab));
  assert(ZXSlabInitStatus::OutOfMemory == Slab::init(&noalloc, 1, &slab));
  assert(ZXSlabInitStatus::Ok == Slab::init(&noalloc, 0, &slab));

  zx_u32 idx = ZX_U32_MAX;
  val_type val = 100;
  assert(ZXSlabInsertStatus::OutOfCapacity == slab.insert(&val, &idx));

  val_type *mut_p = nullptr;
  assert(ZXSlabAccessStatus::OutOfBounds == slab.get_mut(0, &mut_p));

  val_type const *immut_p = nullptr;
  assert(ZXSlabAccessStatus::OutOfBounds == slab.get(0, &immut_p));
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = 1 << 20;

  ZXSliceMut<zx_u8> mem;
  assert(zx_page_alloc_4kb(MEMORY_SIZE, &mem) == ZXPageAllocStatus::Ok);

  noalloc_test();

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

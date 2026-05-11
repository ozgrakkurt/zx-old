#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    zx_panic();
  }
}

static bool tst_get(ZXBitmap const *bmp, zx_u64 idx) {
  bool b = false;
  assert(ZXBitmapAccessStatus::Ok == bmp->get(idx, &b));
  return b;
}

static bool tst_set(ZXBitmap *bmp, zx_u64 idx) {
  bool b = false;
  assert(ZXBitmapAccessStatus::Ok == bmp->set(idx, &b));
  return b;
}

static bool tst_unset(ZXBitmap *bmp, zx_u64 idx) {
  bool b = false;
  assert(ZXBitmapAccessStatus::Ok == bmp->unset(idx, &b));
  return b;
}

struct Ctx {
  ZXAlloc alloc;
};

static void fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXAlloc *alloc = &((Ctx *)usr_ctx)->alloc;
  alloc->reset();

  constexpr zx_u32 max_capacity = 128;
  zx_u32 const capacity =
      zx_max_u32(zx_cast_unsafe_to_u32<zx_u64>(
                     zx_mod_unsafe_u64(fuzz_input->next(), max_capacity)),
                 1);

  constexpr zx_u32 max_n_ops = zx_mul_unsafe_u32(max_capacity, 4);
  zx_u32 const n_ops = zx_cast_unsafe_to_u32<zx_u64>(
      zx_mod_unsafe_u64(fuzz_input->next(), max_n_ops));

  ZXBitmap bmp;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, capacity, &bmp));

  ZXSliceMut<bool> bits;
  assert(ZXAllocStatus::Ok == alloc->alloc<bool>(capacity, &bits));
  bits.fill(false);

  for (zx_u32 i = 0; i < n_ops; i++) {
    zx_u8 const op =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(fuzz_input->next(), 6));
    zx_u32 const idx = zx_cast_unsafe_to_u32<zx_u64>(
        zx_mod_unsafe_u64(fuzz_input->next(), capacity));
    switch (op) {
    // set unsafe
    case 0: {
      assert(bits.read_unsafe(idx) == bmp.set_unsafe(idx));
      bits.write_unsafe(idx, true);
      break;
    }
    // set
    case 1: {
      assert(bits.read_unsafe(idx) == tst_set(&bmp, idx));
      bits.write_unsafe(idx, true);
      break;
    }
    // unset unsafe
    case 2: {
      assert(bits.read_unsafe(idx) == bmp.unset_unsafe(idx));
      bits.write_unsafe(idx, false);
      break;
    }
    // unset
    case 3: {
      assert(bits.read_unsafe(idx) == tst_unset(&bmp, idx));
      bits.write_unsafe(idx, false);
      break;
    }
    // get unsafe
    case 4: {
      assert(bits.read_unsafe(idx) == bmp.get_unsafe(idx));
      break;
    }
    // get
    case 5: {
      assert(bits.read_unsafe(idx) == tst_get(&bmp, idx));
      break;
    }
    default: {
      zx_panic();
    }
    }
  }

  ZXBitmap bmap_copy;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, capacity, &bmap_copy));

  ZXBitmapIter biter = bmp.iter();
  zx_u64 idx = ZX_U32_MAX;
  while (ZXBitmapIterStatus::Continue == biter.next(&idx)) {
    assert(!bmap_copy.set_unsafe(idx));
  }

  for (zx_u32 i = 0; i < capacity; i++) {
    assert(bits.read_unsafe(i) == bmp.get_unsafe(i));
    assert(bits.read_unsafe(i) == bmap_copy.get_unsafe(i));
    assert(bits.read_unsafe(i) == tst_get(&bmp, i));
    assert(bits.read_unsafe(i) == tst_get(&bmap_copy, i));
  }
}

int main(void) {
  constexpr zx_u64 mem_cap = zx_u64{1} << 28;
  ZXSliceMut<zx_u8> mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_4kb(mem_cap, &mem));

  ZXAlloc alloc = ZXAlloc::init(mem);

  constexpr zx_u64 fuzz_mem_cap = zx_u64{1} << 27;
  ZXSliceMut<zx_u8> fuzz_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(fuzz_mem_cap, &fuzz_mem));

  ZXAlloc fuzz_alloc = ZXAlloc::init(fuzz_mem);

  constexpr zx_u64 op_mem_cap = zx_sub_unsafe_u64(mem_cap, fuzz_mem_cap);
  ZXSliceMut<zx_u8> op_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(op_mem_cap, &op_mem));

  ZXAlloc op_alloc = ZXAlloc::init(op_mem);
  Ctx ctx = Ctx{
      .alloc = op_alloc,
  };
  zx_fuzz(&fuzz_alloc, (void *)&ctx, fuzz_one);
  return 0;
}

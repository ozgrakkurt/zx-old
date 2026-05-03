#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXAlloc alloc;
};

static void fuzz_one(void* usr_ctx, ZXFuzzInput* fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXAlloc* alloc = &((Ctx*)usr_ctx)->alloc;
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
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(fuzz_input->next(), 3));
    zx_u32 const idx = zx_cast_unsafe_to_u32<zx_u64>(
        zx_mod_unsafe_u64(fuzz_input->next(), capacity));
    switch (op) {
      // set
      case 0: {
        ZXBitmapAccessStatus const status = bmp.set(idx);
        if (bits.read_unsafe(idx)) {
          assert(ZXBitmapAccessStatus::Set == status);
        } else {
          assert(ZXBitmapAccessStatus::Unset == status);
        }
        bits.write_unsafe(idx, true);

        break;
      }
      // unset
      case 1: {
        ZXBitmapAccessStatus const status = bmp.unset(idx);
        if (bits.read_unsafe(idx)) {
          assert(ZXBitmapAccessStatus::Set == status);
        } else {
          assert(ZXBitmapAccessStatus::Unset == status);
        }
        bits.write_unsafe(idx, false);

        break;
      }
      // get
      case 2: {
        ZXBitmapAccessStatus const status = bmp.get(idx);
        if (bits.read_unsafe(idx)) {
          assert(ZXBitmapAccessStatus::Set == status);
        } else {
          assert(ZXBitmapAccessStatus::Unset == status);
        }

        break;
      }
      default: {
        __builtin_trap();
      }
    }
  }

  ZXBitmap bmap_copy;
  assert(ZXBitmapInitStatus::Ok == ZXBitmap::init(alloc, capacity, &bmap_copy));

  ZXBitmapIter biter = bmp.iter();
  zx_u64 idx = ZX_U32_MAX;
  while (ZXBitmapIterStatus::Continue == biter.next(&idx)) {
    assert(ZXBitmapAccessStatus::Unset == bmap_copy.set(idx));
  }

  for (zx_u32 i = 0; i < capacity; i++) {
    if (bits.read_unsafe(i)) {
      assert(ZXBitmapAccessStatus::Set == bmp.get(i));
      assert(ZXBitmapAccessStatus::Set == bmap_copy.get(i));
    } else {
      assert(ZXBitmapAccessStatus::Unset == bmp.get(i));
      assert(ZXBitmapAccessStatus::Unset == bmap_copy.get(i));
    }
  }
}

int main(void) {
  constexpr zx_u64 mem_cap = zx_u64{1} << 30;
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
  zx_fuzz(&fuzz_alloc, (void*)&ctx, fuzz_one);
  return 0;
}

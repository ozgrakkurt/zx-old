#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slab.h>
#include <zx/zx_slice.h>
#include <zx/zx_xoshiro256.h>

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

  ZXSlab<zx_u64> slab;
  assert(ZXSlabInitStatus ::Ok == ZXSlab<zx_u64>::init(alloc, capacity, &slab));

  ZXSliceMut<zx_u32> slab_keys;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u32>(capacity, &slab_keys));
  slab_keys.fill(ZX_U32_MAX);
  zx_u32 n_slab_keys = 0;

  for (zx_u32 i = 0; i < n_ops; i++) {
    assert(n_slab_keys <= capacity);

    zx_u8 const op =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(fuzz_input->next(), 3));
    bool const rand_key = n_slab_keys == 0 || (fuzz_input->next() & 1) == 1;
    zx_u32 const key_idx =
        zx_rand_mod_u32((zx_u32)fuzz_input->next(), n_slab_keys);
    zx_u32 const key = (rand_key) ? (zx_u32)fuzz_input->next()
                                  : slab_keys.read_unsafe(key_idx);
    switch (op) {
      // insert
      case 0: {
        zx_u64 const to_insert = ZX_U64_MAX;
        zx_u32 new_key = ZX_U32_MAX;
        bool const should_succeed = slab.capacity_left() > 0;
        ZXSlabInsertStatus const status = slab.insert(&to_insert, &new_key);
        if (should_succeed) {
          assert(ZXSlabInsertStatus::Ok == status);
          zx_u64* pp = nullptr;
          assert(ZXSlabAccessStatus::Found == slab.get_mut(new_key, &pp));
          assert(pp != nullptr);
          assert(*pp == ZX_U64_MAX);
          *pp = new_key;
          slab_keys.write_unsafe(n_slab_keys, new_key);
          n_slab_keys = zx_add_unsafe_u32(n_slab_keys, 1);
        } else {
          assert(ZXSlabInsertStatus::OutOfCapacity == status);
        }
        break;
      }
      // remove
      case 1: {
        zx_u64 val = ZX_U64_MAX;
        ZXSlabAccessStatus const status = slab.remove(key, &val);
        if (!rand_key) {
          assert(ZXSlabAccessStatus::Found == status);
          assert(val == key);
          n_slab_keys = zx_sub_unsafe_u32(n_slab_keys, 1);
          slab_keys.write_unsafe(key_idx, slab_keys.read_unsafe(n_slab_keys));
          slab_keys.write_unsafe(n_slab_keys, ZX_U32_MAX);
        } else if (ZXSlabAccessStatus::Found == status) {
          assert(val == key);
          bool found = false;
          for (zx_u32 j = 0; j < n_slab_keys; j++) {
            if (slab_keys.read_unsafe(j) == key) {
              n_slab_keys = zx_sub_unsafe_u32(n_slab_keys, 1);
              slab_keys.write_unsafe(j, slab_keys.read_unsafe(n_slab_keys));
              slab_keys.write_unsafe(n_slab_keys, ZX_U32_MAX);
              found = true;
              break;
            }
          }
          assert(found);
        }
        break;
      }
      // get
      case 2: {
        zx_u64 const* val_p = nullptr;
        ZXSlabAccessStatus const status = slab.get(key, &val_p);
        if (!rand_key) {
          assert(ZXSlabAccessStatus::Found == status);
          assert(val_p != nullptr);
          assert(*val_p == key);
        }
        break;
      }
      default: {
        __builtin_trap();
      }
    }
    for (zx_u32 j = 0; j < n_slab_keys; j++) {
      zx_u64 const* val_p = nullptr;
      zx_u32 const keyk = slab_keys.read_unsafe(j);
      switch (slab.get(keyk, &val_p)) {
        case ZXSlabAccessStatus::Found: {
          break;
        }
        case ZXSlabAccessStatus::NotFound: {
          if (op == 0) {
            __builtin_trap();
          } else if (op == 1) {
            __builtin_trap();

          } else if (op == 2) {
            __builtin_trap();
          } else {
            __builtin_trap();
          }
          break;
        }
        case ZXSlabAccessStatus::OutOfBounds: {
          __builtin_trap();
          break;
        }
        default: {
          __builtin_trap();
          break;
        }
      }
      assert(*val_p == keyk);
    }
  }

  for (zx_u32 i = 0; i < n_slab_keys; i++) {
    zx_u64 val = ZX_U64_MAX;
    zx_u32 const key = slab_keys.read_unsafe(i);
    switch (slab.remove(key, &val)) {
      case ZXSlabAccessStatus::Found: {
        break;
      }
      case ZXSlabAccessStatus::NotFound: {
        __builtin_trap();
        break;
      }
      case ZXSlabAccessStatus::OutOfBounds: {
        __builtin_trap();
        break;
      }
      default: {
        __builtin_trap();
        break;
      }
    }
    assert(val == key);
  }

  assert(slab.n_occupied() == 0);
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

#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXSliceMut<zx_u8> mem;
};

static void fuzz_one(void* usr_ctx, ZXFuzzInput* fuzz_input) {
  assert(usr_ctx != nullptr);
  assert(fuzz_input != nullptr);

  Ctx const* ctx = (Ctx const*)usr_ctx;
  ZXAlloc alloc = ZXAlloc::init(ctx->mem);

  constexpr zx_u64 allocs_cap = 1024;
  ZXSliceMut<zx_u8> allocs[allocs_cap];
  __builtin_memset(allocs, 0, sizeof(allocs));

  zx_u64 const n_allocs = zx_rand_mod_u64(fuzz_input->next(), allocs_cap);

  zx_u64 num_allocs = 0;
  for (zx_u64 i = 0; i < n_allocs; i++) {
    zx_u64 const len = zx_rand_mod_u64(
        fuzz_input->next(), zx_add_unsafe_u64(alloc.capacity_left(), 1));
    zx_u8 const align_kind = zx_cast_unsafe_to_u8(zx_rand_mod_u64(fuzz_input->next(), 8));
    ZXSliceMut<zx_u8> slice;
    ZXAllocStatus status;
    switch (align_kind) {
      case 0: {
        status = alloc.aligned_alloc<zx_u8, 1>(len, &slice);
        break;
      }
      case 1: {
        status = alloc.aligned_alloc<zx_u8, 2>(len, &slice);
        break;
      }
      case 2: {
        status = alloc.aligned_alloc<zx_u8, 4>(len, &slice);
        break;
      }
      case 3: {
        status = alloc.aligned_alloc<zx_u8, 8>(len, &slice);
        break;
      }
      case 4: {
        status = alloc.aligned_alloc<zx_u8, 16>(len, &slice);
        break;
      }
      case 5: {
        status = alloc.aligned_alloc<zx_u8, 32>(len, &slice);
        break;
      }
      case 6: {
        status = alloc.aligned_alloc<zx_u8, 64>(len, &slice);
        break;
      }
      case 7: {
        status = alloc.aligned_alloc<zx_u8, 128>(len, &slice);
        break;
      }
      default: {
        __builtin_trap();
        break;
      }
    }
    if (ZXAllocStatus::Ok == status) {
      zx_u16 const expected_align = zx_shl_unsafe_u16(1, align_kind);
      assert(__builtin_is_aligned(slice.ptr(), expected_align));
      allocs[num_allocs] = slice;
      num_allocs = zx_add_unsafe_u64(num_allocs, 1);
    }
  }

  // make sure no overlap
  for (zx_u64 i = 0; i < num_allocs; i++) {
    for (zx_u64 j = 0; j < i; j++) {
      assert(i != j);
      assert(!allocs[i].as_const().is_overlapping(allocs[j].as_const()));
    }
    for (zx_u64 j = i + 1; j < num_allocs; j++) {
      assert(i != j);
      assert(!allocs[i].as_const().is_overlapping(allocs[j].as_const()));
    }
  }
}

static ZXSliceMut<zx_u8> make_alloc(zx_u64 capacity) {
  assert(zx_is_pow_two_u64(capacity));
  assert(capacity >= 4096);

  ZXSliceMut<zx_u8> alloc;
  ZXPageAllocStatus alloc_status =
      zx_page_alloc_4kb(zx_mul_unsafe_u64(capacity, 2), &alloc);
  assert(alloc_status == ZXPageAllocStatus::Ok);
  ZXSysStatus sys_status =
      zx_sys_mprotect(alloc.slice_at_unsafe(capacity), ZX_SYS_PROT_NONE);
  assert(sys_status == ZXSysStatus::Ok);

  ZXSliceMut<zx_u8> ret = alloc.slice_unsafe(0, capacity);
  ret.memset(0);

  return ret;
}

int main(void) {
  constexpr zx_u64 fuzz_mem_cap = zx_u64{1} << 27;
  ZXSliceMut<zx_u8> fuzz_mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_4kb(fuzz_mem_cap, &fuzz_mem));
  ZXAlloc alloc = ZXAlloc::init(fuzz_mem);
  Ctx const ctx = Ctx{
      .mem = make_alloc(8192),
  };
  zx_fuzz(&alloc, (void*)&ctx, fuzz_one);
  return 0;
}

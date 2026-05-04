#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_xoshiro256.h>
#include "./zx_libc.h"

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXSliceMut<zx_u8> mem;
};

__attribute__((no_builtin)) static bool is_eq(ZXSlice<zx_u8> left,
                                              ZXSlice<zx_u8> right) {
  if (left.len() != right.len()) {
    return false;
  }

  for (zx_u64 i = 0; i < left.len(); i++) {
    if (left.read_unsafe(i) != right.read_unsafe(i)) {
      return false;
    }
  }

  return true;
}

__attribute__((no_builtin)) static void fuzz_one(void* usr_ctx,
                                                 ZXFuzzInput* fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXSliceMut<zx_u8> mem = ((Ctx const*)usr_ctx)->mem;
  assert(mem.len() > 0);

  zx_u64 const len = zx_max_u64(
      1,
      zx_mod_unsafe_u64(fuzz_input->next(), zx_div_unsafe_u64(mem.len(), 2)));

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());
  rand.fill(mem);

  ZXSliceMut<zx_u8> const left = mem.slice_unsafe(0, len);
  ZXSliceMut<zx_u8> const right =
      mem.slice_at_unsafe(zx_sub_unsafe_u64(mem.len(), len));

  assert(left.len() == len);
  assert(right.len() == len);
  assert(!left.as_const().is_overlapping(right.as_const()));

  while (is_eq(left.as_const(), right.as_const())) {
    rand.fill(mem);
  }
  assert(!is_eq(left.as_const(), right.as_const()));
  zx_memcpy(left.ptr(), right.ptr(), len);
  assert(is_eq(left.as_const(), right.as_const()));
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
      .mem = make_alloc(4096),
  };
  zx_fuzz(&alloc, (void*)&ctx, fuzz_one);
  return 0;
}

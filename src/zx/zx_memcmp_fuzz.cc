#include "./zx_libc.h"
#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXSliceMut<zx_u8> mem;
};

// __attribute__((no_builtin)) static inline zx_i32_c dump_memcmp(void const*
// lhs,
//                                                              void const* rhs,
//                                                              zx_u64_c count)
//                                                              {
//   if (count == 0) {
//     return 0;
//   }
//
//   zx_u8 const* left = (zx_u8 const*)lhs;
//   zx_u8 const* right = (zx_u8 const*)rhs;
//
//   for (zx_u64 i=0; i<count; i++) {
//     if (left[i] != right[i]) {
//       return zx_sub_unsafe_i32(zx_i32{left[i]}, zx_i32{right[i]});
//     }
//   }
//
//   return 0;
// }

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 ZXFuzzInput *fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXSliceMut<zx_u8> mem = ((Ctx const *)usr_ctx)->mem;
  assert(mem.len() > 0);

  zx_u64 const len =
      zx_max_u64(1, zx_mod_unsafe_u64(fuzz_input->next(),
                                      zx_div_unsafe_u64(mem.len(), 2)));
  assert(len > 0);

  mem.memset(0);

  ZXSliceMut<zx_u8> const left = mem.slice_unsafe(0, len);
  ZXSliceMut<zx_u8> const right =
      mem.slice_at_unsafe(zx_sub_unsafe_u64(mem.len(), len));

  zx_u64 const idx = zx_mod_unsafe_u64(fuzz_input->next(), len);

  zx_u8 const left_val = (zx_u8)fuzz_input->next();
  zx_u8 const right_val =
      zx_wrapping_add_u8(left_val, (zx_u8)fuzz_input->next());

  left.write_unsafe(idx, left_val);
  right.write_unsafe(idx, right_val);

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());
  if (left_val != right_val) {
    rand.fill(left.slice_at_unsafe(zx_add_unsafe_u64(idx, 1)));
    rand.fill(right.slice_at_unsafe(zx_add_unsafe_u64(idx, 1)));
  }

  assert(left.len() == len);
  assert(right.len() == len);
  assert(!left.as_const().is_overlapping(right.as_const()));
  zx_i32 const res = zx_memcmp(left.ptr(), right.ptr(), len);
  if (left_val == right_val) {
    assert(res == 0);
  } else if (left_val > right_val) {
    assert(res > 0);
  } else {
    assert(res < 0);
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
      .mem = make_alloc(4096),
  };
  zx_fuzz(&alloc, (void *)&ctx, fuzz_one);
  return 0;
}

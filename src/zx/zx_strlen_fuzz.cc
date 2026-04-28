#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include "./zx_libc.h"

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
  ZXSliceMut<zx_u8> mem = ((Ctx const*)usr_ctx)->mem;
  assert(mem.len() > 0);

  bool const left_align = fuzz_input->next() & zx_u64{1};
  zx_u64 const len =
      zx_add_unsafe_u64(zx_mod_unsafe_u64(fuzz_input->next(), mem.len()), 1);
  zx_u64 const first_zero_idx = zx_mod_unsafe_u64(fuzz_input->next(), len);
  zx_u64 const second_zero_idx = zx_add_unsafe_u64(
      first_zero_idx,
      zx_mod_unsafe_u64(fuzz_input->next(),
                        zx_sub_unsafe_u64(len, first_zero_idx)));

  ZXSliceMut<zx_u8> const str =
      left_align ? mem.slice_unsafe(0, len)
                 : mem.slice_at_unsafe(zx_sub_unsafe_u64(mem.len(), len));
  assert(str.len() == len);
  assert(first_zero_idx <= second_zero_idx);
  str.write_unsafe(first_zero_idx, 0);
  str.write_unsafe(second_zero_idx, 0);
  zx_u64 const slen = zx_strlen((char const*)str.ptr());
  assert(slen == first_zero_idx);
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

  return alloc.slice_unsafe(0, capacity);
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

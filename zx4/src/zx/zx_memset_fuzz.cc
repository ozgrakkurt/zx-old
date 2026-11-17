#include "./zx_libc.h"
#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    zx_panic();
  }
}

struct Ctx {
  ZXSliceMut<zx_u8> mem;
};

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 ZXFuzzInput *fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXSliceMut<zx_u8> mem = ((Ctx const *)usr_ctx)->mem;
  assert(mem.len() > 0);

  bool const left_align = fuzz_input->next() & zx_u64{1};
  zx_u64 const len =
      zx_max_u64(1, zx_mod_unsafe_u64(fuzz_input->next(), mem.len()));
  assert(len > 0);

  ZXSliceMut<zx_u8> const str =
      left_align ? mem.slice_unsafe(0, len)
                 : mem.slice_at_unsafe(zx_sub_unsafe_u64(mem.len(), len));
  assert(str.len() == len);
  assert(str.len() > 0);

  // zx_memset(mem.ptr(), 0, mem.len());
  zx_memset(str.ptr(), 69, str.len());

  for (zx_u64 i = 0; i < str.len(); i++) {
    assert(str.read_unsafe(i) == 69);
  }

  if (left_align) {
    for (zx_u64 i = 0; i < str.len(); i++) {
      assert(mem.read_unsafe(i) == 69);
    }
    for (zx_u64 i = str.len(); i < mem.len(); i++) {
      assert(mem.read_unsafe(i) == 0);
    }
  } else {
    zx_u64 const offset = zx_sub_unsafe_u64(mem.len(), str.len());
    for (zx_u64 i = 0; i < offset; i++) {
      assert(mem.read_unsafe(i) == 0);
    }
    for (zx_u64 i = offset; i < mem.len(); i++) {
      assert(mem.read_unsafe(i) == 69);
    }
  }
  zx_memset(str.ptr(), 0, str.len());
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

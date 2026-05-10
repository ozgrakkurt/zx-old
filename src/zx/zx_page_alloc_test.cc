#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>

typedef ZXPageAllocStatus allocfn(zx_u64 size, ZXSliceMut<zx_u8> *out);

static void do_test(allocfn alloc, zx_u64 size) {
  ZXSliceMut<zx_u8> mem;
  ZXPageAllocStatus status = alloc(size, &mem);
  if (status != ZXPageAllocStatus::Ok) {
    __builtin_trap();
  }
  zx_u8 const first = mem.as_const().read_unsafe(0);
  zx_u8 const last = mem.as_const().read_unsafe(zx_sub_unsafe_u64(size, 1));
  mem.write_unsafe(zx_div_unsafe_u64(size, 2), zx_wrapping_add_u8(first, last));

  status = zx_page_free(mem);
  if (status != ZXPageAllocStatus::Ok) {
    __builtin_trap();
  }

  // munmaping an already unmapped region doesn't fail
  status = zx_page_free(mem);
  if (status != ZXPageAllocStatus::Ok) {
    __builtin_trap();
  }
}

int main(void) {
  do_test(zx_page_alloc_4kb, zx_u64{1} << 12);
  do_test(zx_page_alloc_thp_2mb, zx_u64{1} << 21);
  // do_test(zx_page_alloc_huge_2mb, zx_u64{1} << 21);
  // do_test(zx_page_alloc_huge_1gb, zx_u64{1} << 21);

  return 0;
}

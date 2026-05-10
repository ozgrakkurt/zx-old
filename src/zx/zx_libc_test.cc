#include "./zx_libc.h"
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

static void assert(bool test) {
  if (!test) {
    __builtin_trap();
  }
}

__attribute__((no_builtin)) static void check_memcpy(ZXSliceMut<zx_u8> dst,
                                                     ZXSliceMut<zx_u8> src) {
  assert(dst.len() == src.len());

  for (zx_u64 i = 0; i < src.len(); i++) {
    src.write_unsafe(i, (zx_u8)i);
    dst.write_unsafe(i, 69);
  }

  void *out_dst = zx_memcpy(dst.ptr(), src.ptr(), src.len());
  assert(out_dst == dst.ptr());

  for (zx_u64 i = 0; i < dst.len(); i++) {
    assert(src.read_unsafe(i) == dst.read_unsafe(i));
  }
}

__attribute__((no_builtin)) static void check_memset(ZXSliceMut<zx_u8> dst) {
  for (zx_u64 i = 0; i < dst.len(); i++) {
    dst.write_unsafe(i, (zx_u8)i);
  }

  void *out_dst = zx_memset(dst.ptr(), 69, dst.len());
  assert(out_dst == dst.ptr());

  for (zx_u64 i = 0; i < dst.len(); i++) {
    assert(dst.read_unsafe(i) == 69);
  }
}

__attribute__((no_builtin)) static void check_memcmp(ZXSliceMut<zx_u8> left,
                                                     ZXSliceMut<zx_u8> right) {
  // this should have a fuzz test!
  // diff pos first, last, middle. put some diffs after the first diff too
  // (after 5 chars?). lt, gt, eq
  //

  assert(left.len() == right.len());

  for (zx_u64 i = 0; i < left.len(); i++) {
    left.write_unsafe(i, (zx_u8)i);
    right.write_unsafe(i, (zx_u8)i);
  }

  assert(0 == zx_memcmp(left.ptr(), right.ptr(), left.len()));

  // the rest of tests don't make sense for zero length;
  if (left.len() == 0) {
    return;
  }

  zx_u64 const indices[] = {0, zx_sub_unsafe_u64(left.len(), 1),
                            zx_div_unsafe_u64(left.len(), 2)};
  zx_u64 const n_indices = sizeof(indices) / sizeof(indices[0]);

  // lt
  for (zx_u64 i = 0; i < n_indices; i++) {
    zx_u64 const idx = indices[i];
    zx_u8 const orig = left.read_unsafe(idx);
    left.write_unsafe(idx, 69);
    right.write_unsafe(idx, 79);
    assert(0 > zx_memcmp(left.ptr(), right.ptr(), left.len()));
    left.write_unsafe(idx, orig);
    right.write_unsafe(idx, orig);
  }

  // gt
  for (zx_u64 i = 0; i < n_indices; i++) {
    zx_u64 const idx = indices[i];
    zx_u8 const orig = left.read_unsafe(idx);
    left.write_unsafe(idx, 79);
    right.write_unsafe(idx, 69);
    assert(0 < zx_memcmp(left.ptr(), right.ptr(), left.len()));
    left.write_unsafe(idx, orig);
    right.write_unsafe(idx, orig);
  }
}

__attribute__((no_builtin)) static void check_strlen(ZXSliceMut<zx_u8> str) {
  // We can't put a zero byte if the length is zero.
  if (str.len() == 0) {
    return;
  }
  for (zx_u64 i = 0; i < str.len(); i++) {
    str.write_unsafe(i, 69);
  }
  if (str.len() >= 2) {
    zx_u64 const pos = zx_sub_unsafe_u64(str.len(), 2);
    str.write_unsafe(pos, 0);
    assert(pos == zx_strlen((char const *)str.ptr()));
    str.write_unsafe(pos, 69);
  }

  zx_u64 const pos = zx_sub_unsafe_u64(str.len(), 1);
  str.write_unsafe(pos, 0);
  assert(pos == zx_strlen((char const *)str.ptr()));
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
  constexpr zx_u64 capacity = 4096;

  ZXSliceMut<zx_u8> const buf0 = make_alloc(capacity);
  ZXSliceMut<zx_u8> const buf1 = make_alloc(capacity);

  constexpr zx_u64 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129};
  constexpr zx_u64 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u64 i = 0; i < n_lengths; i++) {
    zx_u64 const len = lengths[i];
    zx_u64 const offset = zx_sub_unsafe_u64(capacity, len);
    ZXSliceMut<zx_u8> b0 = buf0.slice_unsafe(offset, len);
    ZXSliceMut<zx_u8> b1 = buf1.slice_unsafe(offset, len);
    check_memcpy(b0, b1);
    check_memset(b0);
    check_memcmp(b0, b1);
    check_strlen(b0);
  }

  return 0;
}

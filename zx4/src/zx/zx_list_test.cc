#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_list.h>
#include <zx/zx_slice.h>

static void test_nocap() {
  ZXAlloc alloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());

  ZXList<zx_u64> list;
  zx_assert_always(ZXListInitStatus::OutOfMemory ==
                   ZXList<zx_u64>::init(&alloc, 1, &list));
  zx_assert_always(ZXListInitStatus::OutOfMemory ==
                   ZXList<zx_u64>::init(&alloc, 69, &list));

  zx_assert_always(ZXListInitStatus::Ok ==
                   ZXList<zx_u64>::init(&alloc, 0, &list));

  zx_u64 val = 69;
  zx_assert_always(ZXListPushStatus::OutOfCapacity == list.push(&val));
  zx_assert_always(ZXListPushStatus::OutOfCapacity == list.push(&val));
}

static void test_smoke(ZXAlloc *alloc, zx_u64 len) {
  ZXList<zx_u64> list;
  zx_assert_always(ZXListInitStatus::Ok ==
                   ZXList<zx_u64>::init(alloc, len, &list));

  for (zx_u64 i = 0; i < len; i++) {
    zx_assert_always(ZXListPushStatus::Ok == list.push(&i));
  }

  zx_assert_always(ZXListPushStatus::OutOfCapacity == list.push(&len));

  ZXSlice<zx_u64> slice = list.slice();
  zx_assert_always(slice.len() == len);
  for (zx_u64 i = 0; i < len; i++) {
    zx_assert_always(slice.read_unsafe(i) == i);
  }
}

static void test_smoke_unsafe(ZXAlloc *alloc, zx_u64 len) {
  ZXList<zx_u64> list;
  zx_assert_always(ZXListInitStatus::Ok ==
                   ZXList<zx_u64>::init(alloc, len, &list));

  for (zx_u64 i = 0; i < len; i++) {
    list.push_unsafe(&i);
  }

  zx_assert_always(ZXListPushStatus::OutOfCapacity == list.push(&len));

  ZXSlice<zx_u64> slice = list.slice();
  zx_assert_always(slice.len() == len);
  for (zx_u64 i = 0; i < len; i++) {
    zx_assert_always(slice.read_unsafe(i) == i);
  }
}

int main() {
  zx_u8 buf[1024];
  constexpr zx_u64 BUF_LEN = sizeof(buf) / sizeof(buf[0]);
  ZXSliceMut<zx_u8> mem = ZXSliceMut<zx_u8>::init_unsafe(buf, BUF_LEN);
  ZXAlloc alloc = ZXAlloc::init(mem);

  test_nocap();

  test_smoke(&alloc, 1);
  alloc.reset();
  test_smoke(&alloc, 10);
  alloc.reset();
  test_smoke(&alloc, 100);
  alloc.reset();
  test_smoke(&alloc, 64);
  alloc.reset();

  test_smoke_unsafe(&alloc, 1);
  alloc.reset();
  test_smoke_unsafe(&alloc, 10);
  alloc.reset();
  test_smoke_unsafe(&alloc, 100);
  alloc.reset();
  test_smoke_unsafe(&alloc, 64);
  alloc.reset();

  return 0;
}

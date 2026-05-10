#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>

static void assert(bool v) {
  if (!v) {
    __builtin_trap();
  }
}

constexpr zx_u64 MEMORY_SIZE = 1 << 16;

void do_test(ZXAlloc *alloc) {
  ZXSliceMut<zx_i32 *> slice;
  ZXAllocStatus status = alloc->alloc<zx_i32 *>(100, &slice);
  assert(status == ZXAllocStatus::Ok);
  assert(slice.len() == 100);
  assert(slice.ptr() != nullptr);
  assert(__builtin_is_aligned(slice.ptr(), alignof(zx_i32 *)));

  for (zx_u64 i = 0; i < 100; i++) {
    status = alloc->create<zx_i32>(slice.offset_unsafe(i));
    assert(status == ZXAllocStatus::Ok);
    assert(slice.read_unsafe(i) != nullptr);
    assert(__builtin_is_aligned(slice.read_unsafe(i), alignof(zx_i32)));
    *slice.read_unsafe(i) = (zx_i32)i;
  }

  for (zx_u64 i = 0; i < 100; i++) {
    assert(*slice.read_unsafe(i) == (zx_i32)i);
  }

  ZXSliceMut<zx_u256> zero_len;
  status = alloc->alloc<zx_u256>(0, &zero_len);
  assert(status == ZXAllocStatus::Ok);
  assert(zero_len.len() == 0);
  assert(zero_len.ptr() != nullptr);
  assert(__builtin_is_aligned(zero_len.ptr(), alignof(zx_u256)));

  {
    ZXSliceMut<zx_u8> big = ZXSliceMut<zx_u8>::init_dangling();
    status = alloc->alloc<zx_u8>(MEMORY_SIZE, &big);
    assert(status == ZXAllocStatus::OutOfMemory);
    assert(big.ptr() == ZXSliceMut<zx_u8>::init_dangling().ptr());
    assert(big.len() == 0);
  }

  alloc->reset();

  {
    ZXSliceMut<zx_u8> big = ZXSliceMut<zx_u8>::init_dangling();
    status = alloc->alloc<zx_u8>(MEMORY_SIZE, &big);
    assert(status == ZXAllocStatus::Ok);
  }

  ZXSliceMut<zx_u8> small = ZXSliceMut<zx_u8>::init_dangling();
  status = alloc->alloc<zx_u8>(1, &small);
  assert(status == ZXAllocStatus::OutOfMemory);
  assert(small.ptr() == ZXSliceMut<zx_u8>::init_dangling().ptr());
  assert(small.len() == 0);

  ZXSliceMut<zx_u256> zero2;
  status = alloc->alloc<zx_u256>(0, &zero2);
  assert(status == ZXAllocStatus::Ok);
  assert(zero2.len() == 0);
  assert(zero2.ptr() != nullptr);
  assert(__builtin_is_aligned(zero2.ptr(), alignof(zx_u256)));
}

static void do_test_nocap() {
  ZXAlloc alloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());

  ZXSliceMut<zx_u256> slice;
  ZXAllocStatus status = alloc.alloc<zx_u256>(0, &slice);
  assert(status == ZXAllocStatus::Ok);
  assert(slice.len() == 0);
  assert(slice.ptr() != nullptr);
  assert(__builtin_is_aligned(slice.ptr(), alignof(zx_u256)));
}

int main(void) {
  ZXSliceMut<zx_u8> mem;
  ZXPageAllocStatus page_status = zx_page_alloc_4kb(MEMORY_SIZE, &mem);
  assert(page_status == ZXPageAllocStatus::Ok);

  do_test_nocap();

  ZXAlloc alloc = ZXAlloc::init(mem);
  do_test(&alloc);

  page_status = zx_page_free(mem);
  assert(page_status == ZXPageAllocStatus::Ok);
  return 0;
}

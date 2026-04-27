#include <zx/zx_int.h>
#include <zx/zx_slice.h>

static void assert(bool v) {
  if (!v) {
    __builtin_trap();
  }
}

static void test_cstr() {
  ZXCString const v = ZXCString::init_unsafe("anan");
  assert(v.slice().len() == 4);
}

static void test_immut() {
  zx_u32 data[20];
  ZXSlice<zx_u32> sli;
  ZXSliceStatus status = ZXSlice<zx_u32>::init(data, 20, &sli);
  assert(status == ZXSliceStatus::Ok);

  for (zx_u64 i = 0; i < 20; i++) {
    data[i] = 100;
  }

  data[10] = 69;
  assert(sli.read_unsafe(10) == 69);

  assert((zx_u64)&data[10] == (zx_u64)sli.offset_unsafe(10));

  zx_u32 const* pp = nullptr;
  status = sli.offset(20, &pp);
  assert(status == ZXSliceStatus::OutOfBounds);

  status = sli.offset(3, &pp);
  assert(status == ZXSliceStatus::Ok);
  assert((zx_u64)&data[3] == (zx_u64)pp);

  ZXSlice<zx_u16> sli16;
  status = sli.cast<zx_u16>(&sli16);
  assert(status == ZXSliceStatus::Ok);
  assert(sli16.len() == zx_mul_unsafe_u64(sli.len(), 2));

  ZXSlice<zx_u8> sli8 = sli.cast_unsafe<zx_u8>();
  assert(sli8.len() == zx_mul_unsafe_u64(sli.len(), 4));

  ZXSlice<zx_u32> slisli = sli.slice_unsafe(0, sli.len() / 2);
  assert(slisli.len() == sli16.len() / 4);

  assert(sli8.read_unaligned_unsafe<zx_u32>(0) == sli.read_unsafe(0));

  assert(sli8.equals(sli.cast_unsafe<zx_u8>()));

  assert(sli8.is_overlapping(sli.cast_unsafe<zx_u8>()));

  ZXSlice<zx_u8> dangler = ZXSlice<zx_u8>::init_dangling();
  assert(!sli8.is_overlapping(dangler));

  assert(dangler.compare(sli8) == ZXSliceCmp::Less);
  assert(sli8.compare(dangler) == ZXSliceCmp::Greater);
  assert(sli8.compare(sli.cast_unsafe<zx_u8>()) == ZXSliceCmp::Equal);
}

static void test_mut() {
  zx_u32 data[20];
  ZXSliceMut<zx_u32> sli;
  ZXSliceStatus status = ZXSliceMut<zx_u32>::init(data, 20, &sli);
  assert(status == ZXSliceStatus::Ok);

  sli.fill(100);

  ZXSliceMut<zx_u32> slik = ZXSliceMut<zx_u32>::init_unsafe(data, 20);
  assert(sli.as_const().equals(slik.as_const()));

  data[10] = 69;
  assert(sli.as_const().read_unsafe(10) == 69);

  sli.write_unsafe(zx_u64{5}, zx_u32{199});
  assert(data[5] == 199);

  assert((zx_u64)&data[10] == (zx_u64)sli.offset_unsafe(10));

  zx_u32* pp = nullptr;
  status = sli.offset(20, &pp);
  assert(status == ZXSliceStatus::OutOfBounds);

  status = sli.offset(3, &pp);
  assert(status == ZXSliceStatus::Ok);
  assert((zx_u64)&data[3] == (zx_u64)pp);

  ZXSliceMut<zx_u16> sli16;
  status = sli.cast<zx_u16>(&sli16);
  assert(status == ZXSliceStatus::Ok);
  assert(sli16.len() == zx_mul_unsafe_u64(sli.len(), 2));

  ZXSliceMut<zx_u8> sli8 = sli.cast_unsafe<zx_u8>();
  assert(sli8.len() == zx_mul_unsafe_u64(sli.len(), 4));

  ZXSliceMut<zx_u32> slisli = sli.slice_unsafe(0, sli.len() / 2);
  assert(slisli.len() == sli16.len() / 4);

  assert(sli8.as_const().read_unaligned_unsafe<zx_u32>(0) ==
         sli.as_const().read_unsafe(0));

  assert(sli8.as_const().equals(sli.cast_unsafe<zx_u8>().as_const()));

  assert(sli8.as_const().is_overlapping(sli.cast_unsafe<zx_u8>().as_const()));

  ZXSliceMut<zx_u8> dangler = ZXSliceMut<zx_u8>::init_dangling();
  assert(!sli8.as_const().is_overlapping(dangler.as_const()));

  assert(dangler.as_const().compare(sli8.as_const()) == ZXSliceCmp::Less);
  assert(sli8.as_const().compare(dangler.as_const()) == ZXSliceCmp::Greater);
  assert(sli8.as_const().compare(sli.cast_unsafe<zx_u8>().as_const()) ==
         ZXSliceCmp::Equal);
}

int main(void) {
  test_mut();
  test_immut();
  test_cstr();

  return 0;
}

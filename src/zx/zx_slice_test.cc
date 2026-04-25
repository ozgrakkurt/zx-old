#include <zx/zx_int.h>
#include <zx/zx_slice.h>

static void assert(bool v) {
  if (!v) {
    __builtin_trap();
  }
}

int main(void) {
  zx_u32 data[20];
  ZXSliceMut<zx_u32> sli;
  ZXSliceStatus status = ZXSliceMut<zx_u32>::init(data, 20, &sli);
  assert(status == ZXSliceStatus::Ok);

  data[10] = 69;
  assert(sli.as_const().read_unsafe(10) == 69);

  sli.write_unsafe(zx_u64{5}, zx_u32{199});
  assert(data[5] == 199);

  assert((zx_u64)&data[10] == (zx_u64)sli.offset_unsafe(10));

  zx_u32* pp = nullptr;
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

  return 0;
}

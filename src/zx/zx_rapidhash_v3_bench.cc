#include <zx/zx_int.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_xoshiro256.h>

int main(void) {
  constexpr zx_u64 len = 569;
  zx_u64 data[len];

  ZXSliceMut<zx_u64> const slice = ZXSliceMut<zx_u64>::init_unsafe(data, len);

  ZXXoshiro256 rand = ZXXoshiro256::init(0);
  rand.fill(slice.cast_unsafe<zx_u8>());

  zx_u64 thash = 0;
  for (zx_u64 i = 0; i < slice.len(); i++) {
    zx_u64 const v = slice.as_const().read_unsafe(i);
    thash = zx_rapidhash_v3(
        ZXSlice<zx_u8>::init_unsafe((zx_u8 const*)&v, sizeof(zx_u64)), thash);
  }

  if (thash == 69)
    __builtin_trap();

  return 0;
}

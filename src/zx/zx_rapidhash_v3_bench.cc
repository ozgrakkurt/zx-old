#include <zx/zx_int.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_rapidhash_v3_bench.h>
#include <zx/zx_xoshiro256.h>

int main(void) {
  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  constexpr zx_u64 len = 20;
  zx_u8 data[len];
  ZXSliceMut<zx_u8> slice = ZXSliceMut<zx_u8>::init_unsafe(data, len);
  rand.fill(slice);

  zx_u64 thash = 0;
  for (zx_u64 i = 0; i < 10; i++) {
    thash = zx_rapidhash_v3(slice.as_const(), thash);
  }

  if (thash == 69) {
    zx_panic();
  }

  return 0;
}

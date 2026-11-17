#include <zx/zx_int.h>
#include <zx/zx_panic.h>
#include <zx/zx_xoshiro256.h>

static void basic() {
  ZXXoshiro256 rand = ZXXoshiro256::init(69);

  zx_u64 const a = rand.next();

  zx_u64 const b = rand.next();

  if (a == b) {
    zx_panic();
  }
}

static void fill_aligned() {
  constexpr zx_u64 len = 69;
  zx_u64 vals[len];
  ZXSliceMut<zx_u64> const data = ZXSliceMut<zx_u64>::init_unsafe(vals, len);
  data.fill(69);

  ZXXoshiro256 rand = ZXXoshiro256::init(69);

  rand.fill(data.cast_unsafe<zx_u8>());

  for (zx_u64 i = 0; i < data.len(); i++) {
    zx_u64 const v = data.as_const().read_unsafe(i);
    if (v == 69) {
      zx_panic();
    }
  }
}

static void fill_partial() {
  constexpr zx_u64 n_bytes = 9;
  zx_u8 bytes[n_bytes];
  ZXSliceMut<zx_u8> const data = ZXSliceMut<zx_u8>::init_unsafe(bytes, n_bytes);
  data.fill(69);

  ZXXoshiro256 rand = ZXXoshiro256::init(69);

  rand.fill(data);

  for (zx_u64 i = 0; i < data.len(); i++) {
    zx_u8 const v = data.as_const().read_unsafe(i);
    if (v == 69) {
      zx_panic();
    }
  }
}

static void fill_long_and_partial() {
  constexpr zx_u64 n_bytes = 69;
  zx_u8 bytes[n_bytes];
  ZXSliceMut<zx_u8> const data = ZXSliceMut<zx_u8>::init_unsafe(bytes, n_bytes);
  data.fill(69);

  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  rand.fill(data);

  for (zx_u64 i = 0; i < data.len(); i++) {
    zx_u8 const v = data.as_const().read_unsafe(i);
    if (v == 69) {
      zx_panic();
    }
  }
}

int main(void) {
  basic();
  fill_aligned();
  fill_partial();
  fill_long_and_partial();

  return 0;
}

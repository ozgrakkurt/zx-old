#include <zx/zx_int.h>
#include <zx/zx_rapidhash_v3.h>

int main(void) {
  constexpr zx_u64 len = 569;
  zx_u8 data[len];

  ZXSliceMut<zx_u8> const slice = ZXSliceMut<zx_u8>::init_unsafe(data, len);

  slice.fill(76);

  for (zx_u64 sl = 0; sl < len; sl++) {
    ZXSliceMut<zx_u8> const s = slice.slice_unsafe(0, sl);
    zx_u64 const base_hash = zx_rapidhash_v3(s.as_const(), 0);
    // check that changing any byte causes the hash to change.
    // This is important because we want to be including every byte in the hash
    // process.
    for (zx_u64 i = 0; i < sl; i++) {
      s.write_unsafe(i, 100);
      zx_u64 const chg_hash = zx_rapidhash_v3(s.as_const(), 0);

      if (base_hash == chg_hash) {
        __builtin_trap();
      }
      s.write_unsafe(i, 76);
    }
  }

  return 0;
}

#include <zx/zx_int.h>

zx_u64 anan(zx_u64 b, zx_u8 c) {
  bool failure = false;
  zx_u64 const out = b.shl(c.bit_width(), &failure);
  if (failure)
    __builtin_trap();
  return out;
}

zx_i128 nn(zx_u64 x) {
  bool failure = false;
  zx_u64 const ret = x.next_pow_two(&failure);
  if (failure)
    __builtin_trap();
  return ret.scast<zx_i128>();
}

zx_u32 nnu(zx_u64 x) {
  return x.bit_width().rtcast_unsafe<zx_u32>().next_pow_two_unsafe();
}

int main(void) { return 0; }

#include <zx/zx_int.h>

static void assert(bool test) {
  if (!test) {
    __builtin_trap();
  }
}

int main(void) {
  // add one over
  bool overflow = false;
  zx_add_u8(ZX_U8_MAX, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_add_u16(ZX_U16_MAX, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_add_u32(ZX_U32_MAX, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_add_u64(ZX_U64_MAX, 1, &overflow);
  assert(overflow);

  // add no overflow
  overflow = false;
  zx_add_u8(69, 31, &overflow);
  assert(!overflow);

  overflow = false;
  zx_add_u16(69, 31, &overflow);
  assert(!overflow);

  overflow = false;
  zx_add_u32(69, 31, &overflow);
  assert(!overflow);

  overflow = false;
  zx_add_u64(69, 31, &overflow);
  assert(!overflow);

  // sub one over
  overflow = false;
  zx_sub_u8(0, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_sub_u16(0, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_sub_u32(0, 1, &overflow);
  assert(overflow);

  overflow = false;
  zx_sub_u64(0, 1, &overflow);
  assert(overflow);

  // sub no overflow
  overflow = false;
  zx_sub_u8(ZX_U8_MAX, 69, &overflow);
  assert(!overflow);

  overflow = false;
  zx_sub_u16(ZX_U16_MAX, 69, &overflow);
  assert(!overflow);

  overflow = false;
  zx_sub_u32(ZX_U32_MAX, 69, &overflow);
  assert(!overflow);

  overflow = false;
  zx_sub_u64(ZX_U64_MAX, 69, &overflow);
  assert(!overflow);

  // mul overflow
  overflow = false;
  zx_mul_u8(ZX_U8_MAX / 2, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_mul_u16(ZX_U16_MAX / 2, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_mul_u32(ZX_U32_MAX / 2, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_mul_u64(ZX_U64_MAX / 2, 3, &overflow);
  assert(overflow);

  // mul no overflow
  overflow = false;
  zx_mul_u8(ZX_U8_MAX / 2, 2, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mul_u16(ZX_U16_MAX / 2, 2, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mul_u32(ZX_U32_MAX / 2, 2, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mul_u64(ZX_U64_MAX / 2, 2, &overflow);
  assert(!overflow);

  // div zero
  overflow = false;
  zx_div_u8(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_u16(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_u32(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_u64(69, 0, &overflow);
  assert(overflow);

  // div nonzero
  overflow = false;
  assert(0 == zx_div_u8(69, 113, &overflow));
  assert(!overflow);

  overflow = false;
  assert(0 == zx_div_u16(69, 1131, &overflow));
  assert(!overflow);

  overflow = false;
  assert(0 == zx_div_u32(69, 1131, &overflow));
  assert(!overflow);

  overflow = false;
  assert(0 == zx_div_u64(69, 1131, &overflow));
  assert(!overflow);

  // mod zero
  overflow = false;
  zx_mod_u8(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_mod_u16(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_mod_u32(69, 0, &overflow);
  assert(overflow);

  overflow = false;
  zx_mod_u64(69, 0, &overflow);
  assert(overflow);

  // mod nonzero
  overflow = false;
  zx_mod_u8(69, 113, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mod_u16(69, 1131, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mod_u32(69, 1131, &overflow);
  assert(!overflow);

  overflow = false;
  zx_mod_u64(69, 1131, &overflow);
  assert(!overflow);

  return 0;
}

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

  // next_mult overflow
  overflow = false;
  zx_next_multiple_of_u8(ZX_U8_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_multiple_of_u16(ZX_U16_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_multiple_of_u32(ZX_U32_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_multiple_of_u64(ZX_U64_MAX, 3, &overflow);
  assert(overflow);

  // next_mult no overflow
  overflow = false;
  assert(3 == zx_next_multiple_of_u8(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(3 == zx_next_multiple_of_u16(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(3 == zx_next_multiple_of_u32(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(3 == zx_next_multiple_of_u64(1, 3, &overflow));
  assert(!overflow);

  // min
  assert(31 == zx_min_u8(31, 69));
  assert(31 == zx_min_u8(69, 31));

  assert(31 == zx_min_u16(31, 69));
  assert(31 == zx_min_u16(69, 31));

  assert(31 == zx_min_u32(31, 69));
  assert(31 == zx_min_u32(69, 31));

  assert(31 == zx_min_u64(31, 69));
  assert(31 == zx_min_u64(69, 31));

  // max
  assert(69 == zx_max_u8(31, 69));
  assert(69 == zx_max_u8(69, 31));

  assert(69 == zx_max_u16(31, 69));
  assert(69 == zx_max_u16(69, 31));

  assert(69 == zx_max_u32(31, 69));
  assert(69 == zx_max_u32(69, 31));

  assert(69 == zx_max_u64(31, 69));
  assert(69 == zx_max_u64(69, 31));

  // shr UB
  bool fail = false;
  zx_shr_u8(1, 8, &fail);
  assert(fail);

  fail = false;
  zx_shr_u16(0, 16, &fail);
  assert(fail);

  fail = false;
  zx_shr_u32(1, 32, &fail);
  assert(fail);

  fail = false;
  zx_shr_u64(0, 64, &fail);
  assert(fail);

  // shr UB more
  fail = false;
  zx_shr_u8(1, 20, &fail);
  assert(fail);

  fail = false;
  zx_shr_u16(1, 30, &fail);
  assert(fail);

  fail = false;
  zx_shr_u32(1, 60, &fail);
  assert(fail);

  fail = false;
  zx_shr_u64(1, ZX_U8_MAX, &fail);
  assert(fail);

  // shr no UB
  fail = false;
  zx_shr_u8(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shr_u16(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shr_u32(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shr_u64(0, 1, &fail);
  assert(!fail);

  // shl UB
  fail = false;
  zx_shl_u8(1, 8, &fail);
  assert(fail);

  fail = false;
  zx_shl_u16(0, 16, &fail);
  assert(fail);

  fail = false;
  zx_shl_u32(1, 32, &fail);
  assert(fail);

  fail = false;
  zx_shl_u64(0, 64, &fail);
  assert(fail);

  // shl UB more
  fail = false;
  zx_shl_u8(1, 20, &fail);
  assert(fail);

  fail = false;
  zx_shl_u16(1, 30, &fail);
  assert(fail);

  fail = false;
  zx_shl_u32(1, 60, &fail);
  assert(fail);

  fail = false;
  zx_shl_u64(1, ZX_U8_MAX, &fail);
  assert(fail);

  // shl no UB
  fail = false;
  zx_shl_u8(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shl_u16(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shl_u32(0, 1, &fail);
  assert(!fail);

  fail = false;
  zx_shl_u64(0, 1, &fail);
  assert(!fail);

  // ctz
  assert(3 == zx_ctz_u8(0b01111000));

  return 0;
}

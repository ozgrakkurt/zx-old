#include <zx/zx_int.h>

static void assert(bool test) {
  if (!test) {
    __builtin_trap();
  }
}

int main(void) {
  // wrapping add
  assert(1 == zx_wrapping_add_u8(ZX_U8_MAX, 2));
  assert(1 == zx_wrapping_add_u16(ZX_U16_MAX, 2));
  assert(1 == zx_wrapping_add_u32(ZX_U32_MAX, 2));
  assert(1 == zx_wrapping_add_u64(ZX_U64_MAX, 2));

  assert(69 == zx_wrapping_add_u8(65, 4));
  assert(69 == zx_wrapping_add_u16(65, 4));
  assert(69 == zx_wrapping_add_u32(65, 4));
  assert(69 == zx_wrapping_add_u64(65, 4));

  // wrapping sub
  assert(1 == zx_wrapping_sub_u8(0, ZX_U8_MAX));
  assert(1 == zx_wrapping_sub_u16(0, ZX_U16_MAX));
  assert(1 == zx_wrapping_sub_u32(0, ZX_U32_MAX));
  assert(1 == zx_wrapping_sub_u64(0, ZX_U64_MAX));

  assert(61 == zx_wrapping_sub_u8(65, 4));
  assert(61 == zx_wrapping_sub_u16(65, 4));
  assert(61 == zx_wrapping_sub_u32(65, 4));
  assert(61 == zx_wrapping_sub_u64(65, 4));

  // wrapping mul
  assert(ZX_U8_MAX - 2__wbu == zx_wrapping_mul_u8(ZX_U8_MAX, 3));
  assert(ZX_U16_MAX - 2__wbu == zx_wrapping_mul_u16(ZX_U16_MAX, 3));
  assert(ZX_U32_MAX - 2__wbu == zx_wrapping_mul_u32(ZX_U32_MAX, 3));
  assert(ZX_U64_MAX - 2__wbu == zx_wrapping_mul_u64(ZX_U64_MAX, 3));

  assert(3 == zx_wrapping_mul_u8(1, 3));
  assert(3 == zx_wrapping_mul_u16(1, 3));
  assert(3 == zx_wrapping_mul_u32(1, 3));
  assert(3 == zx_wrapping_mul_u64(1, 3));

  // saturating add
  assert(ZX_U8_MAX == zx_saturating_add_u8(ZX_U8_MAX - 1, 250));
  assert(ZX_U16_MAX == zx_saturating_add_u16(ZX_U16_MAX - 1, 250));
  assert(ZX_U32_MAX == zx_saturating_add_u32(ZX_U32_MAX - 1, 250));
  assert(ZX_U64_MAX == zx_saturating_add_u64(ZX_U64_MAX - 1, 250));
  assert(2 == zx_saturating_add_u8(1, 1));
  assert(2 == zx_saturating_add_u16(1, 1));
  assert(2 == zx_saturating_add_u32(1, 1));
  assert(2 == zx_saturating_add_u64(1, 1));

  // saturating sub
  assert(0 == zx_saturating_sub_u8(1, 2));
  assert(0 == zx_saturating_sub_u16(1, 2));
  assert(0 == zx_saturating_sub_u32(1, 2));
  assert(0 == zx_saturating_sub_u64(1, 2));

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

  assert(100 == zx_add_unsafe_u8(69, 31));

  overflow = false;
  zx_add_u16(69, 31, &overflow);
  assert(!overflow);

  assert(100 == zx_add_unsafe_u16(69, 31));

  overflow = false;
  zx_add_u32(69, 31, &overflow);
  assert(!overflow);

  assert(100 == zx_add_unsafe_u32(69, 31));

  overflow = false;
  zx_add_u64(69, 31, &overflow);
  assert(!overflow);

  assert(100 == zx_add_unsafe_u64(69, 31));

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

  assert(zx_u8{0} - zx_u8{70} == zx_sub_unsafe_u8(ZX_U8_MAX, 69));

  overflow = false;
  zx_sub_u16(ZX_U16_MAX, 69, &overflow);
  assert(!overflow);

  assert(zx_u16{0} - zx_u16{70} == zx_sub_unsafe_u16(ZX_U16_MAX, 69));

  overflow = false;
  zx_sub_u32(ZX_U32_MAX, 69, &overflow);
  assert(!overflow);

  assert(zx_u32{0} - zx_u32{70} == zx_sub_unsafe_u32(ZX_U32_MAX, 69));

  overflow = false;
  zx_sub_u64(ZX_U64_MAX, 69, &overflow);
  assert(!overflow);

  assert(zx_u64{0} - zx_u64{70} == zx_sub_unsafe_u64(ZX_U64_MAX, 69));

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

  assert(ZX_U8_MAX - 1__wbu == zx_mul_unsafe_u8(ZX_U8_MAX / 2, 2));

  overflow = false;
  zx_mul_u16(ZX_U16_MAX / 2, 2, &overflow);
  assert(!overflow);

  assert(ZX_U16_MAX - 1__wbu == zx_mul_unsafe_u16(ZX_U16_MAX / 2, 2));

  overflow = false;
  zx_mul_u32(ZX_U32_MAX / 2, 2, &overflow);
  assert(!overflow);

  assert(ZX_U32_MAX - 1__wbu == zx_mul_unsafe_u32(ZX_U32_MAX / 2, 2));

  overflow = false;
  zx_mul_u64(ZX_U64_MAX / 2, 2, &overflow);
  assert(!overflow);

  assert(ZX_U64_MAX - 1__wbu == zx_mul_unsafe_u64(ZX_U64_MAX / 2, 2));

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

  assert(0 == zx_div_unsafe_u8(69, 113));

  overflow = false;
  assert(0 == zx_div_u16(69, 1131, &overflow));
  assert(!overflow);

  assert(0 == zx_div_unsafe_u16(69, 1131));

  overflow = false;
  assert(0 == zx_div_u32(69, 1131, &overflow));
  assert(!overflow);

  assert(0 == zx_div_unsafe_u32(69, 1131));

  overflow = false;
  assert(0 == zx_div_u64(69, 1131, &overflow));
  assert(!overflow);

  assert(0 == zx_div_unsafe_u64(69, 1131));

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

  assert(69__wbu == zx_mod_unsafe_u8(69, 113));

  overflow = false;
  zx_mod_u16(69, 1131, &overflow);
  assert(!overflow);

  assert(69__wbu == zx_mod_unsafe_u16(69, 113));

  overflow = false;
  zx_mod_u32(69, 1131, &overflow);
  assert(!overflow);

  assert(69__wbu == zx_mod_unsafe_u32(69, 113));

  overflow = false;
  zx_mod_u64(69, 1131, &overflow);
  assert(!overflow);

  assert(69__wbu == zx_mod_unsafe_u64(69, 113));

  // div_ceil overflow
  overflow = false;
  zx_div_ceil_u8(ZX_U8_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_ceil_u16(ZX_U16_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_ceil_u32(ZX_U32_MAX, 3, &overflow);
  assert(overflow);

  overflow = false;
  zx_div_ceil_u64(ZX_U64_MAX, 3, &overflow);
  assert(overflow);

  // div_ceil no overflow
  overflow = false;
  assert(1 == zx_div_ceil_u8(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(2 == zx_div_ceil_u8(4, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_div_ceil_u16(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(2 == zx_div_ceil_u16(4, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_div_ceil_u32(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(2 == zx_div_ceil_u32(4, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_div_ceil_u64(1, 3, &overflow));
  assert(!overflow);

  overflow = false;
  assert(2 == zx_div_ceil_u64(4, 3, &overflow));
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

  assert(3 == zx_next_multiple_of_unsafe_u8(1, 3));

  overflow = false;
  assert(3 == zx_next_multiple_of_u16(1, 3, &overflow));
  assert(!overflow);

  assert(3 == zx_next_multiple_of_unsafe_u16(1, 3));

  overflow = false;
  assert(3 == zx_next_multiple_of_u32(1, 3, &overflow));
  assert(!overflow);

  assert(3 == zx_next_multiple_of_unsafe_u32(1, 3));

  overflow = false;
  assert(3 == zx_next_multiple_of_u64(1, 3, &overflow));
  assert(!overflow);

  assert(3 == zx_next_multiple_of_unsafe_u64(1, 3));

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

  assert(0 == zx_shr_unsafe_u8(0, 1));
  assert(1 == zx_shr_unsafe_u8(4, 2));

  fail = false;
  zx_shr_u16(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shr_unsafe_u16(0, 1));
  assert(1 == zx_shr_unsafe_u16(4, 2));

  fail = false;
  zx_shr_u32(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shr_unsafe_u32(0, 1));
  assert(1 == zx_shr_unsafe_u32(4, 2));

  fail = false;
  zx_shr_u64(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shr_unsafe_u64(0, 1));
  assert(1 == zx_shr_unsafe_u64(4, 2));

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

  assert(0 == zx_shl_unsafe_u8(0, 1));
  assert(4 == zx_shl_unsafe_u8(1, 2));

  fail = false;
  zx_shl_u16(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shl_unsafe_u16(0, 1));
  assert(4 == zx_shl_unsafe_u16(1, 2));

  fail = false;
  zx_shl_u32(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shl_unsafe_u32(0, 1));
  assert(4 == zx_shl_unsafe_u32(1, 2));

  fail = false;
  zx_shl_u64(0, 1, &fail);
  assert(!fail);

  assert(0 == zx_shl_unsafe_u64(0, 1));
  assert(4 == zx_shl_unsafe_u64(1, 2));

  // ctz
  assert(3 == zx_ctz_u8(0b01111000));
  assert(3 == zx_ctz_u16(0b01111000));
  assert(3 == zx_ctz_u32(0b01111000));
  assert(3 == zx_ctz_u64(0b01111000));
  assert(8 == zx_ctz_u8(0));
  assert(16 == zx_ctz_u16(0));
  assert(32 == zx_ctz_u32(0));
  assert(64 == zx_ctz_u64(0));

  // clz
  assert(1 == zx_clz_u8(0b01111000));
  assert(9 == zx_clz_u16(0b01111000));
  assert(25 == zx_clz_u32(0b01111000));
  assert(57 == zx_clz_u64(0b01111000));
  assert(8 == zx_clz_u8(0));
  assert(16 == zx_clz_u16(0));
  assert(32 == zx_clz_u32(0));
  assert(64 == zx_clz_u64(0));

  // popcnt
  assert(4 == zx_popcnt_u8(0b01111000));
  assert(4 == zx_popcnt_u16(0b01111000));
  assert(4 == zx_popcnt_u32(0b01111000));
  assert(4 == zx_popcnt_u64(0b01111000));
  assert(0 == zx_popcnt_u8(0));
  assert(0 == zx_popcnt_u16(0));
  assert(0 == zx_popcnt_u32(0));
  assert(0 == zx_popcnt_u64(0));
  assert(8 == zx_popcnt_u8(ZX_U8_MAX));
  assert(16 == zx_popcnt_u16(ZX_U16_MAX));
  assert(32 == zx_popcnt_u32(ZX_U32_MAX));
  assert(64 == zx_popcnt_u64(ZX_U64_MAX));

  // bit_width
  assert(7 == zx_bit_width_u8(0b01111000));
  assert(7 == zx_bit_width_u16(0b01111000));
  assert(7 == zx_bit_width_u32(0b01111000));
  assert(7 == zx_bit_width_u64(0b01111000));
  assert(0 == zx_bit_width_u8(0));
  assert(0 == zx_bit_width_u16(0));
  assert(0 == zx_bit_width_u32(0));
  assert(0 == zx_bit_width_u64(0));
  assert(8 == zx_bit_width_u8(ZX_U8_MAX));
  assert(16 == zx_bit_width_u16(ZX_U16_MAX));
  assert(32 == zx_bit_width_u32(ZX_U32_MAX));
  assert(64 == zx_bit_width_u64(ZX_U64_MAX));

  // next_pow_two
  overflow = false;
  assert(128 == zx_next_pow_two_u8(100, &overflow));
  assert(!overflow);

  overflow = false;
  assert(128 == zx_next_pow_two_u16(100, &overflow));
  assert(!overflow);

  overflow = false;
  assert(128 == zx_next_pow_two_u32(100, &overflow));
  assert(!overflow);

  overflow = false;
  assert(128 == zx_next_pow_two_u64(100, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_next_pow_two_u8(0, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_next_pow_two_u16(0, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_next_pow_two_u32(0, &overflow));
  assert(!overflow);

  overflow = false;
  assert(1 == zx_next_pow_two_u64(0, &overflow));
  assert(!overflow);

  // next_pow_two overflow
  overflow = false;
  zx_next_pow_two_u8(ZX_U8_MAX, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_pow_two_u16(ZX_U16_MAX, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_pow_two_u32(ZX_U32_MAX, &overflow);
  assert(overflow);

  overflow = false;
  zx_next_pow_two_u64(ZX_U64_MAX, &overflow);
  assert(overflow);

  // is_pow_two
  assert(zx_is_pow_two_u8(1));
  assert(zx_is_pow_two_u16(1));
  assert(zx_is_pow_two_u32(1));
  assert(zx_is_pow_two_u64(1));

  assert(!zx_is_pow_two_u8(0));
  assert(!zx_is_pow_two_u16(0));
  assert(!zx_is_pow_two_u32(0));
  assert(!zx_is_pow_two_u64(0));

  // inverse
  assert(zx_u8{123} + zx_inverse_u8(123) == 0);
  assert(zx_u16{123} + zx_inverse_u16(123) == 0);
  assert(zx_u32{123} + zx_inverse_u32(123) == 0);
  assert(zx_u64{123} + zx_inverse_u64(123) == 0);

  // cast to u8
  overflow = false;
  assert(69 == zx_cast_to_u8<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_u8>(zx_u8{69}));

  assert(69 == zx_cast_to_u8<zx_u16>(zx_u16{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_u16>(zx_u16{69}));

  assert(69 == zx_cast_to_u8<zx_u32>(zx_u32{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_u32>(zx_u32{69}));

  assert(69 == zx_cast_to_u8<zx_u64>(zx_u64{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_u64>(zx_u64{69}));

  assert(69 == zx_cast_to_u8<zx_i8>(zx_i8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_i8>(zx_i8{69}));

  assert(69 == zx_cast_to_u8<zx_i16>(zx_i16{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_i16>(zx_i16{69}));

  assert(69 == zx_cast_to_u8<zx_i32>(zx_i32{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_i32>(zx_i32{69}));

  assert(69 == zx_cast_to_u8<zx_i64>(zx_i64{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u8<zx_i64>(zx_i64{69}));

  // cast to u16
  overflow = false;
  assert(69 == zx_cast_to_u16<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u16<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_u16<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_u16>(zx_u16{1100}));

  assert(1100 == zx_cast_to_u16<zx_u32>(zx_u32{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_u32>(zx_u32{1100}));

  assert(1100 == zx_cast_to_u16<zx_u64>(zx_u64{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_u64>(zx_u64{1100}));

  assert(120 == zx_cast_to_u16<zx_i8>(zx_i8{120}, &overflow));
  assert(!overflow);
  assert(120 == zx_cast_unsafe_to_u16<zx_i8>(zx_i8{120}));

  assert(1100 == zx_cast_to_u16<zx_i16>(zx_i16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_i16>(zx_i16{1100}));

  assert(1100 == zx_cast_to_u16<zx_i32>(zx_i32{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_i32>(zx_i32{1100}));

  assert(1100 == zx_cast_to_u16<zx_i64>(zx_i64{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u16<zx_i64>(zx_i64{1100}));

  // cast to u32
  overflow = false;
  assert(69 == zx_cast_to_u32<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u32<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_u32<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u32<zx_u16>(zx_u16{1100}));

  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_u32<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_u32<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu));

  assert(zx_u64{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_u32<zx_u64>(zx_u64{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_u32<zx_u64>(zx_u64{ZX_U16_MAX} + 1__wbu));

  assert(120 == zx_cast_to_u32<zx_i8>(zx_i8{120}, &overflow));
  assert(!overflow);
  assert(120 == zx_cast_unsafe_to_u32<zx_i8>(zx_i8{120}));

  assert(1100 == zx_cast_to_u32<zx_i16>(zx_i16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u32<zx_i16>(zx_i16{1100}));

  assert(zx_i32{ZX_U16_MAX} + 1__wb ==
         zx_cast_to_u32<zx_i32>(zx_i32{ZX_U16_MAX} + 1__wb, &overflow));
  assert(!overflow);
  assert(zx_i32{ZX_U16_MAX} + 1__wb ==
         zx_cast_unsafe_to_u32<zx_i32>(zx_i32{ZX_U16_MAX} + 1__wb));

  assert(ZX_U16_MAX + 1 ==
         zx_cast_to_u32<zx_i64>(zx_i64{ZX_U16_MAX + 1}, &overflow));
  assert(!overflow);
  assert(zx_i64{ZX_U16_MAX} + 1__wb ==
         zx_cast_unsafe_to_u32<zx_i64>(zx_i64{ZX_U16_MAX} + 1__wb));

  // cast to u64
  overflow = false;
  assert(69 == zx_cast_to_u64<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_u64<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_u64<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u64<zx_u16>(zx_u16{1100}));

  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_u64<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_u64<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu));

  assert(ZX_U32_MAX + zx_u64{1} ==
         zx_cast_to_u64<zx_u64>(zx_u64{ZX_U32_MAX + zx_u64{1}}, &overflow));
  assert(!overflow);
  assert(zx_u64{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_u64<zx_u64>(zx_u64{ZX_U16_MAX} + 1__wbu));

  assert(120 == zx_cast_to_u64<zx_i8>(zx_i8{120}, &overflow));
  assert(!overflow);
  assert(120 == zx_cast_unsafe_to_u64<zx_i8>(zx_i8{120}));

  assert(1100 == zx_cast_to_u64<zx_i16>(zx_i16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_u64<zx_i16>(zx_i16{1100}));

  assert(zx_i32{ZX_U16_MAX} + 1__wb ==
         zx_cast_to_u64<zx_i32>(zx_i32{ZX_U16_MAX} + 1__wb, &overflow));
  assert(!overflow);
  assert(zx_i32{ZX_U16_MAX} + 1__wb ==
         zx_cast_unsafe_to_u64<zx_i32>(zx_i32{ZX_U16_MAX} + 1__wb));

  assert(ZX_U32_MAX + zx_u64{1} ==
         zx_cast_to_u64<zx_i64>(zx_i64{ZX_U32_MAX + zx_u64{1}}, &overflow));
  assert(!overflow);
  assert(zx_i64{ZX_U16_MAX} + 1__wb ==
         zx_cast_unsafe_to_u64<zx_i64>(zx_i64{ZX_U16_MAX} + 1__wb));

  // cast to i8
  overflow = false;
  assert(69 == zx_cast_to_i8<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i8<zx_u8>(zx_u8{69}));

  assert(69 == zx_cast_to_i8<zx_u16>(zx_u16{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i8<zx_u16>(zx_u16{69}));

  assert(69 == zx_cast_to_i8<zx_u32>(zx_u32{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i8<zx_u32>(zx_u32{69}));

  assert(69 == zx_cast_to_i8<zx_u64>(zx_u64{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i8<zx_u64>(zx_u64{69}));

  assert(-69 == zx_cast_to_i8<zx_i8>(zx_i8{-69}, &overflow));
  assert(!overflow);
  assert(-69 == zx_cast_unsafe_to_i8<zx_i8>(zx_i8{-69}));

  assert(-69 == zx_cast_to_i8<zx_i16>(zx_i16{-69}, &overflow));
  assert(!overflow);
  assert(-69 == zx_cast_unsafe_to_i8<zx_i16>(zx_i16{-69}));

  assert(-69 == zx_cast_to_i8<zx_i32>(zx_i32{-69}, &overflow));
  assert(!overflow);
  assert(-69 == zx_cast_unsafe_to_i8<zx_i32>(zx_i32{-69}));

  assert(-69 == zx_cast_to_i8<zx_i64>(zx_i64{-69}, &overflow));
  assert(!overflow);
  assert(-69 == zx_cast_unsafe_to_i8<zx_i64>(zx_i64{-69}));

  // cast to i16
  overflow = false;
  assert(69 == zx_cast_to_i16<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i16<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_i16<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_i16<zx_u16>(zx_u16{1100}));

  assert(1100 == zx_cast_to_i16<zx_u32>(zx_u32{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_i16<zx_u32>(zx_u32{1100}));

  assert(1100 == zx_cast_to_i16<zx_u64>(zx_u64{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_i16<zx_u64>(zx_u64{1100}));

  assert(-120 == zx_cast_to_i16<zx_i8>(zx_i8{-120}, &overflow));
  assert(!overflow);
  assert(-120 == zx_cast_unsafe_to_i16<zx_i8>(zx_i8{-120}));

  assert(-1100 == zx_cast_to_i16<zx_i16>(zx_i16{-1100}, &overflow));
  assert(!overflow);
  assert(-1100 == zx_cast_unsafe_to_i16<zx_i16>(zx_i16{-1100}));

  assert(-1100 == zx_cast_to_i16<zx_i32>(zx_i32{-1100}, &overflow));
  assert(!overflow);
  assert(-1100 == zx_cast_unsafe_to_i16<zx_i32>(zx_i32{-1100}));

  assert(-1100 == zx_cast_to_i16<zx_i64>(zx_i64{-1100}, &overflow));
  assert(!overflow);
  assert(-1100 == zx_cast_unsafe_to_i16<zx_i64>(zx_i64{-1100}));

  // cast to i32
  overflow = false;
  assert(69 == zx_cast_to_i32<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i32<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_i32<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_i32<zx_u16>(zx_u16{1100}));

  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_i32<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_i32<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu));

  assert(zx_u64{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_i32<zx_u64>(zx_u64{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u64{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_i32<zx_u64>(zx_u64{ZX_U16_MAX} + 1__wbu));

  assert(-120 == zx_cast_to_i32<zx_i8>(zx_i8{-120}, &overflow));
  assert(!overflow);
  assert(-120 == zx_cast_unsafe_to_i32<zx_i8>(zx_i8{-120}));

  assert(-1100 == zx_cast_to_i32<zx_i16>(zx_i16{-1100}, &overflow));
  assert(!overflow);
  assert(-1100 == zx_cast_unsafe_to_i32<zx_i16>(zx_i16{-1100}));

  assert(-65537 == zx_cast_to_i32<zx_i32>(zx_i32{-65537}, &overflow));
  assert(!overflow);
  assert(-65537 == zx_cast_unsafe_to_i32<zx_i32>(zx_i32{-65537}));

  assert(-65537 == zx_cast_to_i32<zx_i64>(zx_i64{-65537}, &overflow));
  assert(!overflow);
  assert(-65537 == zx_cast_unsafe_to_i32<zx_i64>(zx_i64{-65537}));

  // cast to i64
  overflow = false;
  assert(69 == zx_cast_to_i64<zx_u8>(zx_u8{69}, &overflow));
  assert(!overflow);
  assert(69 == zx_cast_unsafe_to_i64<zx_u8>(zx_u8{69}));

  assert(1100 == zx_cast_to_i64<zx_u16>(zx_u16{1100}, &overflow));
  assert(!overflow);
  assert(1100 == zx_cast_unsafe_to_i64<zx_u16>(zx_u16{1100}));

  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_to_i64<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u32{ZX_U16_MAX} + 1__wbu ==
         zx_cast_unsafe_to_i64<zx_u32>(zx_u32{ZX_U16_MAX} + 1__wbu));

  assert(zx_u64{ZX_U32_MAX} + 1__wbu ==
         zx_cast_to_i64<zx_u64>(zx_u64{ZX_U32_MAX} + 1__wbu, &overflow));
  assert(!overflow);
  assert(zx_u64{ZX_U32_MAX} + 1__wbu ==
         zx_cast_unsafe_to_i64<zx_u64>(zx_u64{ZX_U32_MAX} + 1__wbu));

  assert(-120 == zx_cast_to_i64<zx_i8>(zx_i8{-120}, &overflow));
  assert(!overflow);
  assert(-120 == zx_cast_unsafe_to_i64<zx_i8>(zx_i8{-120}));

  assert(-1100 == zx_cast_to_i64<zx_i16>(zx_i16{-1100}, &overflow));
  assert(!overflow);
  assert(-1100 == zx_cast_unsafe_to_i64<zx_i16>(zx_i16{-1100}));

  assert(-65537 == zx_cast_to_i64<zx_i32>(zx_i32{-65537}, &overflow));
  assert(!overflow);
  assert(-65537 == zx_cast_unsafe_to_i64<zx_i32>(zx_i32{-65537}));

  assert(-4294967297__wb ==
         zx_cast_to_i64<zx_i64>(zx_i64{-4294967297__wb}, &overflow));
  assert(!overflow);
  assert(-4294967297__wb ==
         zx_cast_unsafe_to_i64<zx_i64>(zx_i64{-4294967297__wb}));

  // cast to u8 overflow
  overflow = false;
  zx_cast_to_u8<zx_u16>(zx_u16{300}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_u32>(zx_u32{300}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_u64>(zx_u64{300}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i8>(zx_i8{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i16>(zx_i16{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i16>(zx_i16{300}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i32>(zx_i32{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i32>(zx_i32{300}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i64>(zx_i64{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u8<zx_i64>(zx_i64{300}, &overflow);
  assert(overflow);

  // cast to u16 overflow
  overflow = false;
  zx_cast_to_u16<zx_u32>(zx_u32{70000}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_u64>(zx_u64{70000}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i8>(zx_i8{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i16>(zx_i16{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i32>(zx_i32{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i32>(zx_i32{70000}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i64>(zx_i64{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u16<zx_i64>(zx_i64{70000}, &overflow);
  assert(overflow);

  // cast to u32 overflow
  overflow = false;
  zx_cast_to_u32<zx_u64>(zx_u64{ZX_U32_MAX + zx_u64{1}}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u32<zx_i8>(zx_i8{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u32<zx_i16>(zx_i16{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u32<zx_i32>(zx_i32{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u32<zx_i64>(zx_i64{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u32<zx_i64>(zx_i64{ZX_U32_MAX + zx_i64{1}}, &overflow);
  assert(overflow);

  // cast to u64 overflow
  overflow = false;
  zx_cast_to_u64<zx_u128>(zx_u128{ZX_U64_MAX} + 1__wbu, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u64<zx_i8>(zx_i8{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u64<zx_i16>(zx_i16{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u64<zx_i32>(zx_i32{-1}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_u64<zx_i64>(zx_i64{-1}, &overflow);
  assert(overflow);

  // cast to i8 overflow
  overflow = false;
  zx_cast_to_i8<zx_u8>(zx_u8{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_u16>(zx_u16{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_u32>(zx_u32{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_u64>(zx_u64{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i16>(zx_i16{-129}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i16>(zx_i16{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i32>(zx_i32{-129}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i32>(zx_i32{128}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i64>(zx_i64{-129}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i8<zx_i64>(zx_i64{128}, &overflow);
  assert(overflow);

  // cast to i16 overflow
  overflow = false;
  zx_cast_to_i16<zx_u16>(zx_u16{32768}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_u32>(zx_u32{32768}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_u64>(zx_u64{32768}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_i32>(zx_i32{-32769}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_i32>(zx_i32{32768}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_i64>(zx_i64{-32769}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i16<zx_i64>(zx_i64{32768}, &overflow);
  assert(overflow);

  // cast to i32 overflow
  overflow = false;
  zx_cast_to_i32<zx_u32>(zx_u32{2147483648}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i32<zx_u64>(zx_u64{2147483648}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i32<zx_i64>(zx_i64{-2147483649__wb}, &overflow);
  assert(overflow);

  overflow = false;
  zx_cast_to_i32<zx_i64>(zx_i64{2147483648__wb}, &overflow);
  assert(overflow);

  // cast to i64 overflow
  overflow = false;
  zx_cast_to_i64<zx_u64>(zx_u64{9223372036854775808__wbu}, &overflow);
  assert(overflow);

  return 0;
}

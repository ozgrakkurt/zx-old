pub const OverflowErr = error{
    IntegerOverflow,
};

pub fn add_g(comptime T: type, a: T, b: T) OverflowErr!T {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_u8(a: u8, b: u8) OverflowErr!u8 {
    return try add_g(u8, a, b);
}

pub fn add_u16(a: u16, b: u16) OverflowErr!u16 {
    return try add_g(u16, a, b);
}

pub fn add_u32(a: u32, b: u32) OverflowErr!u32 {
    return try add_g(u32, a, b);
}

pub fn add_u64(a: u64, b: u64) OverflowErr!u64 {
    return try add_g(u64, a, b);
}

pub fn add_i8(a: i8, b: i8) OverflowErr!i8 {
    return try add_g(i8, a, b);
}

pub fn add_i16(a: i16, b: i16) OverflowErr!i16 {
    return try add_g(i16, a, b);
}

pub fn add_i32(a: i32, b: i32) OverflowErr!i32 {
    return try add_g(i32, a, b);
}

pub fn add_i64(a: i64, b: i64) OverflowErr!i64 {
    return try add_g(i64, a, b);
}

pub fn wrapping_add_g(comptime T: type, a: T, b: T) T {
    return a +% b;
}

pub fn wrapping_add_u8(a: u8, b: u8) u8 {
    return a +% b;
}

pub fn wrapping_add_u16(a: u16, b: u16) u16 {
    return a +% b;
}

pub fn wrapping_add_u32(a: u32, b: u32) u32 {
    return a +% b;
}

pub fn wrapping_add_u64(a: u64, b: u64) u64 {
    return a +% b;
}

pub fn saturating_add_g(comptime T: type, a: T, b: T) T {
    return a +| b;
}

pub fn saturating_add_u8(a: u8, b: u8) u8 {
    return a +| b;
}

pub fn saturating_add_u16(a: u16, b: u16) u16 {
    return a +| b;
}

pub fn saturating_add_u32(a: u32, b: u32) u32 {
    return a +| b;
}

pub fn saturating_add_u64(a: u64, b: u64) u64 {
    return a +| b;
}

pub fn saturating_add_i8(a: i8, b: i8) i8 {
    return a +| b;
}

pub fn saturating_add_i16(a: i16, b: i16) i16 {
    return a +| b;
}

pub fn saturating_add_i32(a: i32, b: i32) i32 {
    return a +| b;
}

pub fn saturating_add_i64(a: i64, b: i64) i64 {
    return a +| b;
}

pub fn add_unsafe_g(comptime T: type, a: T, b: T) T {
    return a + b;
}

pub fn add_unsafe_u8(a: u8, b: u8) u8 {
    return a + b;
}

pub fn add_unsafe_u16(a: u16, b: u16) u16 {
    return a + b;
}

pub fn add_unsafe_u32(a: u32, b: u32) u32 {
    return a + b;
}

pub fn add_unsafe_u64(a: u64, b: u64) u64 {
    return a + b;
}

pub fn add_unsafe_i8(a: i8, b: i8) i8 {
    return a + b;
}

pub fn add_unsafe_i16(a: i16, b: i16) i16 {
    return a + b;
}

pub fn add_unsafe_i32(a: i32, b: i32) i32 {
    return a + b;
}

pub fn add_unsafe_i64(a: i64, b: i64) i64 {
    return a + b;
}

pub fn sub_g(comptime T: type, a: T, b: T) OverflowErr!T {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_u8(a: u8, b: u8) OverflowErr!u8 {
    return try sub_g(u8, a, b);
}

pub fn sub_u16(a: u16, b: u16) OverflowErr!u16 {
    return try sub_g(u16, a, b);
}

pub fn sub_u32(a: u32, b: u32) OverflowErr!u32 {
    return try sub_g(u32, a, b);
}

pub fn sub_u64(a: u64, b: u64) OverflowErr!u64 {
    return try sub_g(u64, a, b);
}

pub fn sub_i8(a: i8, b: i8) OverflowErr!i8 {
    return try sub_g(i8, a, b);
}

pub fn sub_i16(a: i16, b: i16) OverflowErr!i16 {
    return try sub_g(i16, a, b);
}

pub fn sub_i32(a: i32, b: i32) OverflowErr!i32 {
    return try sub_g(i32, a, b);
}

pub fn sub_i64(a: i64, b: i64) OverflowErr!i64 {
    return try sub_g(i64, a, b);
}

pub fn wrapping_sub_g(comptime T: type, a: T, b: T) T {
    return a -% b;
}

pub fn wrapping_sub_u8(a: u8, b: u8) u8 {
    return a -% b;
}

pub fn wrapping_sub_u16(a: u16, b: u16) u16 {
    return a -% b;
}

pub fn wrapping_sub_u32(a: u32, b: u32) u32 {
    return a -% b;
}

pub fn wrapping_sub_u64(a: u64, b: u64) u64 {
    return a -% b;
}

pub fn saturating_sub_g(comptime T: type, a: T, b: T) T {
    return a -| b;
}

pub fn saturating_sub_u8(a: u8, b: u8) u8 {
    return a -| b;
}

pub fn saturating_sub_u16(a: u16, b: u16) u16 {
    return a -| b;
}

pub fn saturating_sub_u32(a: u32, b: u32) u32 {
    return a -| b;
}

pub fn saturating_sub_u64(a: u64, b: u64) u64 {
    return a -| b;
}

pub fn saturating_sub_i8(a: i8, b: i8) i8 {
    return a -| b;
}

pub fn saturating_sub_i16(a: i16, b: i16) i16 {
    return a -| b;
}

pub fn saturating_sub_i32(a: i32, b: i32) i32 {
    return a -| b;
}

pub fn saturating_sub_i64(a: i64, b: i64) i64 {
    return a -| b;
}

pub fn sub_unsafe_g(comptime T: type, a: T, b: T) T {
    return a - b;
}

pub fn sub_unsafe_u8(a: u8, b: u8) u8 {
    return a - b;
}

pub fn sub_unsafe_u16(a: u16, b: u16) u16 {
    return a - b;
}

pub fn sub_unsafe_u32(a: u32, b: u32) u32 {
    return a - b;
}

pub fn sub_unsafe_u64(a: u64, b: u64) u64 {
    return a - b;
}

pub fn sub_unsafe_i8(a: i8, b: i8) i8 {
    return a - b;
}

pub fn sub_unsafe_i16(a: i16, b: i16) i16 {
    return a - b;
}

pub fn sub_unsafe_i32(a: i32, b: i32) i32 {
    return a - b;
}

pub fn sub_unsafe_i64(a: i64, b: i64) i64 {
    return a - b;
}

pub fn mul_g(comptime T: type, a: T, b: T) OverflowErr!T {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_u8(a: u8, b: u8) OverflowErr!u8 {
    return try mul_g(u8, a, b);
}

pub fn mul_u16(a: u16, b: u16) OverflowErr!u16 {
    return try mul_g(u16, a, b);
}

pub fn mul_u32(a: u32, b: u32) OverflowErr!u32 {
    return try mul_g(u32, a, b);
}

pub fn mul_u64(a: u64, b: u64) OverflowErr!u64 {
    return try mul_g(u64, a, b);
}

pub fn mul_i8(a: i8, b: i8) OverflowErr!i8 {
    return try mul_g(i8, a, b);
}

pub fn mul_i16(a: i16, b: i16) OverflowErr!i16 {
    return try mul_g(i16, a, b);
}

pub fn mul_i32(a: i32, b: i32) OverflowErr!i32 {
    return try mul_g(i32, a, b);
}

pub fn mul_i64(a: i64, b: i64) OverflowErr!i64 {
    return try mul_g(i64, a, b);
}

pub fn wrapping_mul_g(comptime T: type, a: T, b: T) T {
    return a *% b;
}

pub fn wrapping_mul_u8(a: u8, b: u8) u8 {
    return a *% b;
}

pub fn wrapping_mul_u16(a: u16, b: u16) u16 {
    return a *% b;
}

pub fn wrapping_mul_u32(a: u32, b: u32) u32 {
    return a *% b;
}

pub fn wrapping_mul_u64(a: u64, b: u64) u64 {
    return a *% b;
}

pub fn saturating_mul_g(comptime T: type, a: T, b: T) T {
    return a *| b;
}

pub fn saturating_mul_u8(a: u8, b: u8) u8 {
    return a *| b;
}

pub fn saturating_mul_u16(a: u16, b: u16) u16 {
    return a *| b;
}

pub fn saturating_mul_u32(a: u32, b: u32) u32 {
    return a *| b;
}

pub fn saturating_mul_u64(a: u64, b: u64) u64 {
    return a *| b;
}

pub fn mul_unsafe_g(comptime T: type, a: T, b: T) T {
    return a * b;
}

pub fn mul_unsafe_u8(a: u8, b: u8) u8 {
    return a * b;
}

pub fn mul_unsafe_u16(a: u16, b: u16) u16 {
    return a * b;
}

pub fn mul_unsafe_u32(a: u32, b: u32) u32 {
    return a * b;
}

pub fn mul_unsafe_u64(a: u64, b: u64) u64 {
    return a * b;
}

pub fn mul_unsafe_i8(a: i8, b: i8) i8 {
    return a * b;
}

pub fn mul_unsafe_i16(a: i16, b: i16) i16 {
    return a * b;
}

pub fn mul_unsafe_i32(a: i32, b: i32) i32 {
    return a * b;
}

pub fn mul_unsafe_i64(a: i64, b: i64) i64 {
    return a * b;
}

pub const DivErr = error{
    DivisionByZero,
};

pub fn div_g(comptime T: type, a: T, b: T) DivErr!T {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

pub fn div_u8(a: u8, b: u8) DivErr!u8 {
    return try div_g(u8, a, b);
}

pub fn div_u16(a: u16, b: u16) DivErr!u16 {
    return try div_g(u16, a, b);
}

pub fn div_u32(a: u32, b: u32) DivErr!u32 {
    return try div_g(u32, a, b);
}

pub fn div_u64(a: u64, b: u64) DivErr!u64 {
    return try div_g(u64, a, b);
}

pub fn div_unsafe_g(comptime T: type, a: T, b: T) T {
    return a / b;
}

pub fn div_unsafe_u8(a: u8, b: u8) u8 {
    return a / b;
}

pub fn div_unsafe_u16(a: u16, b: u16) u16 {
    return a / b;
}

pub fn div_unsafe_u32(a: u32, b: u32) u32 {
    return a / b;
}

pub fn div_unsafe_u64(a: u64, b: u64) u64 {
    return a / b;
}

pub const ModErr = error{
    DivisionByZero,
};

pub fn mod_g(comptime T: type, a: T, b: T) ModErr!T {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

pub fn mod_u8(a: u8, b: u8) ModErr!u8 {
    return try mod_g(u8, a, b);
}

pub fn mod_u16(a: u16, b: u16) ModErr!u16 {
    return try mod_g(u16, a, b);
}

pub fn mod_u32(a: u32, b: u32) ModErr!u32 {
    return try mod_g(u32, a, b);
}

pub fn mod_u64(a: u64, b: u64) ModErr!u64 {
    return try mod_g(u64, a, b);
}

pub fn mod_unsafe_g(comptime T: type, a: T, b: T) T {
    return a % b;
}

pub fn mod_unsafe_u8(a: u8, b: u8) u8 {
    return a % b;
}

pub fn mod_unsafe_u16(a: u16, b: u16) u16 {
    return a % b;
}

pub fn mod_unsafe_u32(a: u32, b: u32) u32 {
    return a % b;
}

pub fn mod_unsafe_u64(a: u64, b: u64) u64 {
    return a % b;
}

pub const DivCeilErr = error{
    IntegerOverflow,
    DivisionByZero,
};

pub fn div_ceil_g(comptime T: type, a: T, b: T) DivCeilErr!T {
    const c = try add_g(T, a, b);
    const d = try sub_g(T, c, 1);
    return try div_g(T, d, b);
}

pub fn div_ceil_u8(a: u8, b: u8) DivCeilErr!u8 {
    return try div_ceil_g(u8, a, b);
}

pub fn div_ceil_u16(a: u16, b: u16) DivCeilErr!u16 {
    return try div_ceil_g(u16, a, b);
}

pub fn div_ceil_u32(a: u32, b: u32) DivCeilErr!u32 {
    return try div_ceil_g(u32, a, b);
}

pub fn div_ceil_u64(a: u64, b: u64) DivCeilErr!u64 {
    return try div_ceil_g(u64, a, b);
}

pub fn div_ceil_unsafe_g(comptime T: type, a: T, b: T) T {
    const c = add_unsafe_g(T, a, b);
    const d = sub_unsafe_g(T, c, 1);
    return div_unsafe_g(T, d, b);
}

pub fn div_ceil_unsafe_u8(a: u8, b: u8) u8 {
    return div_ceil_unsafe_g(u8, a, b);
}

pub fn div_ceil_unsafe_u16(a: u16, b: u16) u16 {
    return div_ceil_unsafe_g(u16, a, b);
}

pub fn div_ceil_unsafe_u32(a: u32, b: u32) u32 {
    return div_ceil_unsafe_g(u32, a, b);
}

pub fn div_ceil_unsafe_u64(a: u64, b: u64) u64 {
    return div_ceil_unsafe_g(u64, a, b);
}

pub const NextMulOfErr = error{
    IntegerOverflow,
    DivisionByZero,
};

pub fn next_multiple_of_g(comptime T: type, a: T, b: T) NextMulOfErr!T {
    const c = try div_ceil_g(T, a, b);
    return try mul_g(T, c, b);
}

pub fn next_multiple_of_u8(a: u8, b: u8) NextMulOfErr!u8 {
    return try next_multiple_of_g(u8, a, b);
}

pub fn next_multiple_of_u16(a: u16, b: u16) NextMulOfErr!u16 {
    return try next_multiple_of_g(u16, a, b);
}

pub fn next_multiple_of_u32(a: u32, b: u32) NextMulOfErr!u32 {
    return try next_multiple_of_g(u32, a, b);
}

pub fn next_multiple_of_u64(a: u64, b: u64) NextMulOfErr!u64 {
    return try next_multiple_of_g(u64, a, b);
}

pub fn next_multiple_of_unsafe_g(comptime T: type, a: T, b: T) T {
    const c = div_ceil_unsafe_g(T, a, b);
    return mul_unsafe_g(T, c, b);
}

pub fn next_multiple_of_unsafe_u8(a: u8, b: u8) u8 {
    return next_multiple_of_unsafe_g(u8, a, b);
}

pub fn next_multiple_of_unsafe_u16(a: u16, b: u16) u16 {
    return next_multiple_of_unsafe_g(u16, a, b);
}

pub fn next_multiple_of_unsafe_u32(a: u32, b: u32) u32 {
    return next_multiple_of_unsafe_g(u32, a, b);
}

pub fn next_multiple_of_unsafe_u64(a: u64, b: u64) u64 {
    return next_multiple_of_unsafe_g(u64, a, b);
}

pub fn min_g(comptime T: type, a: T, b: T) T {
    return @min(a, b);
}

pub fn min_u8(a: u8, b: u8) u8 {
    return min_g(u8, a, b);
}

pub fn min_u16(a: u16, b: u16) u16 {
    return min_g(u16, a, b);
}

pub fn min_u32(a: u32, b: u32) u32 {
    return min_g(u32, a, b);
}

pub fn min_u64(a: u64, b: u64) u64 {
    return min_g(u64, a, b);
}

pub fn min_i8(a: i8, b: i8) i8 {
    return min_g(i8, a, b);
}

pub fn min_i16(a: i16, b: i16) i16 {
    return min_g(i16, a, b);
}

pub fn min_i32(a: i32, b: i32) i32 {
    return min_g(i32, a, b);
}

pub fn min_i64(a: i64, b: i64) i64 {
    return min_g(i64, a, b);
}

pub fn max_g(comptime T: type, a: T, b: T) T {
    return @max(a, b);
}

pub fn max_u8(a: u8, b: u8) u8 {
    return max_g(u8, a, b);
}

pub fn max_u16(a: u16, b: u16) u16 {
    return max_g(u16, a, b);
}

pub fn max_u32(a: u32, b: u32) u32 {
    return max_g(u32, a, b);
}

pub fn max_u64(a: u64, b: u64) u64 {
    return max_g(u64, a, b);
}

pub fn max_i8(a: i8, b: i8) i8 {
    return max_g(i8, a, b);
}

pub fn max_i16(a: i16, b: i16) i16 {
    return max_g(i16, a, b);
}

pub fn max_i32(a: i32, b: i32) i32 {
    return max_g(i32, a, b);
}

pub fn max_i64(a: i64, b: i64) i64 {
    return max_g(i64, a, b);
}

pub fn ctz_g(comptime T: type, a: T) u8 {
    return @ctz(a);
}

pub fn ctz_u8(a: u8) u8 {
    return ctz_g(u8, a);
}

pub fn ctz_u16(a: u16) u8 {
    return ctz_g(u16, a);
}

pub fn ctz_u32(a: u32) u8 {
    return ctz_g(u32, a);
}

pub fn ctz_u64(a: u64) u8 {
    return ctz_g(u64, a);
}

pub fn clz_g(comptime T: type, a: T) u8 {
    return @clz(a);
}

pub fn clz_u8(a: u8) u8 {
    return clz_g(u8, a);
}

pub fn clz_u16(a: u16) u8 {
    return clz_g(u16, a);
}

pub fn clz_u32(a: u32) u8 {
    return clz_g(u32, a);
}

pub fn clz_u64(a: u64) u8 {
    return clz_g(u64, a);
}

pub fn popcnt_g(comptime T: type, a: T) u8 {
    return @popCount(a);
}

pub fn popcnt_u8(a: u8) u8 {
    return popcnt_g(u8, a);
}

pub fn popcnt_u16(a: u16) u8 {
    return popcnt_g(u16, a);
}

pub fn popcnt_u32(a: u32) u8 {
    return popcnt_g(u32, a);
}

pub fn popcnt_u64(a: u64) u8 {
    return popcnt_g(u64, a);
}

pub fn bit_width_g(comptime T: type, a: T) u8 {
    return @bitSizeOf(T) - clz_g(T, a);
}

pub fn bit_width_u8(a: u8) u8 {
    return bit_width_g(u8, a);
}

pub fn bit_width_u16(a: u16) u8 {
    return bit_width_g(u16, a);
}

pub fn bit_width_u32(a: u32) u8 {
    return bit_width_g(u32, a);
}

pub fn bit_width_u64(a: u64) u8 {
    return bit_width_g(u64, a);
}

// pub fn Log2Int(comptime T: type) type {
//     if (T == comptime_int) {
//         return comptime_int;
//     }
//     const log2_bits = 16 - @clz(@bitSizeOf(T) - 1);
//     return @Int(.unsigned, log2_bits);
// }

pub const ShiftErr = error{
    IllegalShift,
};

pub fn shl_g(comptime T: type, a: T, amount: u64) ShiftErr!T {
    if (amount >= @bitSizeOf(T)) {
        @branchHint(.cold);
        return BitShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shr_g(comptime T: type, a: T, amount: u64) BitShiftErr!T {
    if (amount >= @bitSizeOf(T)) {
        @branchHint(.cold);
        return BitShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

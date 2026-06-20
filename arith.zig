const safety = @import("./safety.zig").safety;
const assert = @import("./assert.zig").assert;

const U8_MAX = ~@as(u8, 0);
const U16_MAX = ~@as(u16, 0);
const U32_MAX = ~@as(u32, 0);
const U64_MAX = ~@as(u64, 0);
const U128_MAX = ~@as(u128, 0);
const U256_MAX = ~@as(u256, 0);

const I8_MAX = @as(i8, U8_MAX >> 1);
const I16_MAX = @as(i16, U16_MAX >> 1);
const I32_MAX = @as(i32, U32_MAX >> 1);
const I64_MAX = @as(i64, U64_MAX >> 1);
const I128_MAX = @as(i128, U128_MAX >> 1);
const I256_MAX = @as(i256, U256_MAX >> 1);

const I8_MIN = ~I8_MAX;
const I16_MIN = ~I16_MAX;
const I32_MIN = ~I32_MAX;
const I64_MIN = ~I64_MAX;
const I128_MIN = ~I128_MAX;
const I256_MIN = ~I256_MAX;

pub const OverflowErr = error{
    IntegerOverflow,
};

pub fn add_u8(a: u8, b: u8) OverflowErr!u8 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_u8 {
    assert(OverflowErr.IntegerOverflow == add_u8(U8_MAX, 1));
    assert(U8_MAX == add_u8(U8_MAX, 0) catch unreachable);
    assert(8 == add_u8(3, 5) catch unreachable);
}

pub fn add_u16(a: u16, b: u16) OverflowErr!u16 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_i8(a: i8, b: i8) OverflowErr!i8 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_i8 {
    assert(OverflowErr.IntegerOverflow == add_i8(I8_MAX, 1));
    assert(I8_MAX == add_i8(I8_MAX, 0) catch unreachable);
    assert(13 == add_i8(-3, 16) catch unreachable);
}

pub fn add_i16(a: i16, b: i16) OverflowErr!i16 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn add_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn wrapping_add_u8(a: u8, b: u8) u8 {
    return a +% b;
}

test wrapping_add_u8 {
    assert(1 == wrapping_add_u8(U8_MAX, 2));
    assert(5 == wrapping_add_u8(2, 3));
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

pub fn saturating_add_u8(a: u8, b: u8) u8 {
    return a +| b;
}

test saturating_add_u8 {
    assert(U8_MAX == saturating_add_u8(U8_MAX, 5));
    assert(5 == saturating_add_u8(2, 3));
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

pub fn add_unsafe_u8(a: u8, b: u8) u8 {
    return a + b;
}

test add_unsafe_u8 {
    assert(5 == add_unsafe_u8(3, 5));
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

pub fn sub_u8(a: u8, b: u8) OverflowErr!u8 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_u16(a: u16, b: u16) OverflowErr!u16 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_i8(a: i8, b: i8) OverflowErr!i8 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_i16(a: i16, b: i16) OverflowErr!i16 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn sub_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
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

pub fn mul_u8(a: u8, b: u8) OverflowErr!u8 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_u16(a: u16, b: u16) OverflowErr!u16 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_i8(a: i8, b: i8) OverflowErr!i8 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_i16(a: i16, b: i16) OverflowErr!i16 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

pub fn mul_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
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

pub fn div_u8(a: u8, b: u8) DivErr!u8 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

pub fn div_u16(a: u16, b: u16) DivErr!u16 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

pub fn div_u32(a: u32, b: u32) DivErr!u32 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

pub fn div_u64(a: u64, b: u64) DivErr!u64 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
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

pub fn mod_u8(a: u8, b: u8) ModErr!u8 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

pub fn mod_u16(a: u16, b: u16) ModErr!u16 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

pub fn mod_u32(a: u32, b: u32) ModErr!u32 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

pub fn mod_u64(a: u64, b: u64) ModErr!u64 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
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

pub fn div_ceil_u8(a: u8, b: u8) DivCeilErr!u8 {
    const c = try add_u8(a, b);
    const d = try sub_u8(c, 1);
    return try div_u8(d, b);
}

pub fn div_ceil_u16(a: u16, b: u16) DivCeilErr!u16 {
    const c = try add_u16(a, b);
    const d = try sub_u16(c, 1);
    return try div_u16(d, b);
}

pub fn div_ceil_u32(a: u32, b: u32) DivCeilErr!u32 {
    const c = try add_u32(a, b);
    const d = try sub_u32(c, 1);
    return try div_u32(d, b);
}

pub fn div_ceil_u64(a: u64, b: u64) DivCeilErr!u64 {
    const c = try add_u64(a, b);
    const d = try sub_u64(c, 1);
    return try div_u64(d, b);
}

pub fn div_ceil_unsafe_u8(a: u8, b: u8) u8 {
    const c = add_unsafe_u8(a, b);
    const d = sub_unsafe_u8(c, 1);
    return div_unsafe_u8(d, b);
}

pub fn div_ceil_unsafe_u16(a: u16, b: u16) u16 {
    const c = add_unsafe_u16(a, b);
    const d = sub_unsafe_u16(c, 1);
    return div_unsafe_u16(d, b);
}

pub fn div_ceil_unsafe_u32(a: u32, b: u32) u32 {
    const c = add_unsafe_u32(a, b);
    const d = sub_unsafe_u32(c, 1);
    return div_unsafe_u32(d, b);
}

pub fn div_ceil_unsafe_u64(a: u64, b: u64) u64 {
    const c = add_unsafe_u64(a, b);
    const d = sub_unsafe_u64(c, 1);
    return div_unsafe_u64(d, b);
}

pub const NextMulOfErr = error{
    IntegerOverflow,
    DivisionByZero,
};

pub fn next_multiple_of_u8(a: u8, b: u8) NextMulOfErr!u8 {
    const c = try div_ceil_u8(a, b);
    return try mul_u8(c, b);
}

pub fn next_multiple_of_u16(a: u16, b: u16) NextMulOfErr!u16 {
    const c = try div_ceil_u16(a, b);
    return try mul_u16(c, b);
}

pub fn next_multiple_of_u32(a: u32, b: u32) NextMulOfErr!u32 {
    const c = try div_ceil_u32(a, b);
    return try mul_u32(c, b);
}

pub fn next_multiple_of_u64(a: u64, b: u64) NextMulOfErr!u64 {
    const c = try div_ceil_u64(a, b);
    return try mul_u64(c, b);
}

pub fn next_multiple_of_unsafe_u8(a: u8, b: u8) u8 {
    const c = div_ceil_unsafe_u8(a, b);
    return mul_unsafe_u8(c, b);
}

pub fn next_multiple_of_unsafe_u16(a: u16, b: u16) u16 {
    const c = div_ceil_unsafe_u16(a, b);
    return mul_unsafe_u16(c, b);
}

pub fn next_multiple_of_unsafe_u32(a: u32, b: u32) u32 {
    const c = div_ceil_unsafe_u32(a, b);
    return mul_unsafe_u32(c, b);
}

pub fn next_multiple_of_unsafe_u64(a: u64, b: u64) u64 {
    const c = div_ceil_unsafe_u64(a, b);
    return mul_unsafe_u64(c, b);
}

pub fn min_u8(a: u8, b: u8) u8 {
    return @min(a, b);
}

pub fn min_u16(a: u16, b: u16) u16 {
    return @min(a, b);
}

pub fn min_u32(a: u32, b: u32) u32 {
    return @min(a, b);
}

pub fn min_u64(a: u64, b: u64) u64 {
    return @min(a, b);
}

pub fn min_i8(a: i8, b: i8) i8 {
    return @min(a, b);
}

pub fn min_i16(a: i16, b: i16) i16 {
    return @min(a, b);
}

pub fn min_i32(a: i32, b: i32) i32 {
    return @min(a, b);
}

pub fn min_i64(a: i64, b: i64) i64 {
    return @min(a, b);
}

pub fn max_u8(a: u8, b: u8) u8 {
    return @max(a, b);
}

pub fn max_u16(a: u16, b: u16) u16 {
    return @max(a, b);
}

pub fn max_u32(a: u32, b: u32) u32 {
    return @max(a, b);
}

pub fn max_u64(a: u64, b: u64) u64 {
    return @max(a, b);
}

pub fn max_i8(a: i8, b: i8) i8 {
    return @max(a, b);
}

pub fn max_i16(a: i16, b: i16) i16 {
    return @max(a, b);
}

pub fn max_i32(a: i32, b: i32) i32 {
    return @max(a, b);
}

pub fn max_i64(a: i64, b: i64) i64 {
    return @max(a, b);
}

pub fn ctz_u8(a: u8) u8 {
    return @ctz(a);
}

pub fn ctz_u16(a: u16) u8 {
    return @ctz(a);
}

pub fn ctz_u32(a: u32) u8 {
    return @ctz(a);
}

pub fn ctz_u64(a: u64) u8 {
    return @ctz(a);
}

pub fn clz_u8(a: u8) u8 {
    return @clz(a);
}

pub fn clz_u16(a: u16) u8 {
    return @clz(a);
}

pub fn clz_u32(a: u32) u8 {
    return @clz(a);
}

pub fn clz_u64(a: u64) u8 {
    return @clz(a);
}

pub fn popcnt_u8(a: u8) u8 {
    return @popCount(a);
}

pub fn popcnt_u16(a: u16) u8 {
    return @popCount(a);
}

pub fn popcnt_u32(a: u32) u8 {
    return @popCount(a);
}

pub fn popcnt_u64(a: u64) u8 {
    return @popCount(a);
}

pub fn bit_width_u8(a: u8) u8 {
    return 8 - @clz(a);
}

pub fn bit_width_u16(a: u16) u8 {
    return 16 - @clz(a);
}

pub fn bit_width_u32(a: u32) u8 {
    return 32 - @clz(a);
}

pub fn bit_width_u64(a: u64) u8 {
    return 64 - @clz(a);
}

pub const ShiftErr = error{
    IllegalShift,
};

pub fn shl_u8(a: u8, amount: u64) ShiftErr!u8 {
    if (amount >= 8) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_u16(a: u16, amount: u64) ShiftErr!u16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_u32(a: u32, amount: u64) ShiftErr!u32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_u64(a: u64, amount: u64) ShiftErr!u64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_i8(a: i8, amount: u64) ShiftErr!i8 {
    if (amount >= 8) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_i16(a: i16, amount: u64) ShiftErr!i16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_i32(a: i32, amount: u64) ShiftErr!i32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shl_i64(a: i64, amount: u64) ShiftErr!i64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

pub fn shr_u8(a: u8, amount: u64) ShiftErr!u8 {
    if (amount >= 8) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_u16(a: u16, amount: u64) ShiftErr!u16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_u32(a: u32, amount: u64) ShiftErr!u32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_u64(a: u64, amount: u64) ShiftErr!u64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_i8(a: i8, amount: u64) ShiftErr!i8 {
    if (amount >= 8) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_i16(a: i16, amount: u64) ShiftErr!i16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_i32(a: i32, amount: u64) ShiftErr!i32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shr_i64(a: i64, amount: u64) ShiftErr!i64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a >> @truncate(amount);
}

pub fn shl_unsafe_u8(a: u8, amount: u64) u8 {
    if (safety) {
        if (amount >= 8) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_u16(a: u16, amount: u64) u16 {
    if (safety) {
        if (amount >= 16) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_u32(a: u32, amount: u64) u32 {
    if (safety) {
        if (amount >= 32) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_u64(a: u64, amount: u64) u64 {
    if (safety) {
        if (amount >= 64) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_i8(a: i8, amount: u64) i8 {
    if (safety) {
        if (amount >= 8) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_i16(a: i16, amount: u64) i16 {
    if (safety) {
        if (amount >= 16) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_i32(a: i32, amount: u64) i32 {
    if (safety) {
        if (amount >= 32) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shl_unsafe_i64(a: i64, amount: u64) i64 {
    if (safety) {
        if (amount >= 64) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a << @truncate(amount);
}

pub fn shr_unsafe_u8(a: u8, amount: u64) u8 {
    if (safety) {
        if (amount >= 8) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_u16(a: u16, amount: u64) u16 {
    if (safety) {
        if (amount >= 16) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_u32(a: u32, amount: u64) u32 {
    if (safety) {
        if (amount >= 32) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_u64(a: u64, amount: u64) u64 {
    if (safety) {
        if (amount >= 64) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_i8(a: i8, amount: u64) i8 {
    if (safety) {
        if (amount >= 8) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_i16(a: i16, amount: u64) i16 {
    if (safety) {
        if (amount >= 16) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_i32(a: i32, amount: u64) i32 {
    if (safety) {
        if (amount >= 32) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn shr_unsafe_i64(a: i64, amount: u64) i64 {
    if (safety) {
        if (amount >= 64) {
            @branchHint(.cold);
            @panic("illegal shift");
        }
    }
    return a >> @truncate(amount);
}

pub fn next_pow_two_u8(a: u8) OverflowErr!u8 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u8(a - 1);

    return shl_u8(1, shift) catch {
        return OverflowErr.IntegerOverflow;
    };
}

pub fn next_pow_two_u16(a: u16) OverflowErr!u16 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u16(a - 1);

    return shl_u16(1, shift) catch {
        return OverflowErr.IntegerOverflow;
    };
}

pub fn next_pow_two_u32(a: u32) OverflowErr!u32 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u32(a - 1);

    return shl_u32(1, shift) catch {
        return OverflowErr.IntegerOverflow;
    };
}

pub fn next_pow_two_u64(a: u64) OverflowErr!u64 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u64(a - 1);

    return shl_u64(1, shift) catch {
        return OverflowErr.IntegerOverflow;
    };
}

pub fn next_pow_two_unsafe_u8(a: u8) u8 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u8(a - 1);

    return shl_unsafe_u8(1, shift);
}

pub fn next_pow_two_unsafe_u16(a: u16) u16 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u16(a - 1);

    return shl_unsafe_u16(1, shift);
}

pub fn next_pow_two_unsafe_u32(a: u32) u32 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u32(a - 1);

    return shl_unsafe_u32(1, shift);
}

pub fn next_pow_two_unsafe_u64(a: u64) u64 {
    if (a == 0) {
        return 1;
    }

    const shift: u8 = bit_width_u64(a - 1);

    return shl_unsafe_u64(1, shift);
}

pub fn is_pow_two_u8(a: u8) bool {
    return @popCount(a) == 1;
}

pub fn is_pow_two_u16(a: u16) bool {
    return @popCount(a) == 1;
}

pub fn is_pow_two_u32(a: u32) bool {
    return @popCount(a) == 1;
}

pub fn is_pow_two_u64(a: u64) bool {
    return @popCount(a) == 1;
}

pub fn inverse_u8(a: u8) u8 {
    return ~a +% 1;
}

pub fn inverse_u16(a: u16) u16 {
    return ~a +% 1;
}

pub fn inverse_u32(a: u32) u32 {
    return ~a +% 1;
}

pub fn inverse_u64(a: u64) u64 {
    return ~a +% 1;
}

pub fn inverse_i8(a: i8) i8 {
    return ~a +% 1;
}

pub fn inverse_i16(a: i16) i16 {
    return ~a +% 1;
}

pub fn inverse_i32(a: i32) i32 {
    return ~a +% 1;
}

pub fn inverse_i64(a: i64) i64 {
    return ~a +% 1;
}

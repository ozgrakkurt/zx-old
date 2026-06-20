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

test add_u16 {
    assert(OverflowErr.IntegerOverflow == add_u16(U16_MAX, 1));
    assert(U16_MAX == add_u16(U16_MAX, 0) catch unreachable);
    assert(10 == add_u16(4, 6) catch unreachable);
}

pub fn add_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_u32 {
    assert(OverflowErr.IntegerOverflow == add_u32(U32_MAX, 1));
    assert(U32_MAX == add_u32(U32_MAX, 0) catch unreachable);
    assert(10 == add_u32(4, 6) catch unreachable);
}

pub fn add_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_u64 {
    assert(OverflowErr.IntegerOverflow == add_u64(U64_MAX, 1));
    assert(U64_MAX == add_u64(U64_MAX, 0) catch unreachable);
    assert(10 == add_u64(4, 6) catch unreachable);
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

test add_i16 {
    assert(OverflowErr.IntegerOverflow == add_i16(I16_MAX, 1));
    assert(I16_MAX == add_i16(I16_MAX, 0) catch unreachable);
    assert(13 == add_i16(-3, 16) catch unreachable);
}

pub fn add_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_i32 {
    assert(OverflowErr.IntegerOverflow == add_i32(I32_MAX, 1));
    assert(I32_MAX == add_i32(I32_MAX, 0) catch unreachable);
    assert(13 == add_i32(-3, 16) catch unreachable);
}

pub fn add_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @addWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test add_i64 {
    assert(OverflowErr.IntegerOverflow == add_i64(I64_MAX, 1));
    assert(I64_MAX == add_i64(I64_MAX, 0) catch unreachable);
    assert(13 == add_i64(-3, 16) catch unreachable);
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

test wrapping_add_u16 {
    assert(1 == wrapping_add_u16(U16_MAX, 2));
    assert(5 == wrapping_add_u16(2, 3));
}

pub fn wrapping_add_u32(a: u32, b: u32) u32 {
    return a +% b;
}

test wrapping_add_u32 {
    assert(1 == wrapping_add_u32(U32_MAX, 2));
    assert(5 == wrapping_add_u32(2, 3));
}

pub fn wrapping_add_u64(a: u64, b: u64) u64 {
    return a +% b;
}

test wrapping_add_u64 {
    assert(1 == wrapping_add_u64(U64_MAX, 2));
    assert(5 == wrapping_add_u64(2, 3));
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

test saturating_add_u16 {
    assert(U16_MAX == saturating_add_u16(U16_MAX, 5));
    assert(5 == saturating_add_u16(2, 3));
}

pub fn saturating_add_u32(a: u32, b: u32) u32 {
    return a +| b;
}

test saturating_add_u32 {
    assert(U32_MAX == saturating_add_u32(U32_MAX, 5));
    assert(5 == saturating_add_u32(2, 3));
}

pub fn saturating_add_u64(a: u64, b: u64) u64 {
    return a +| b;
}

test saturating_add_u64 {
    assert(U64_MAX == saturating_add_u64(U64_MAX, 5));
    assert(5 == saturating_add_u64(2, 3));
}

pub fn saturating_add_i8(a: i8, b: i8) i8 {
    return a +| b;
}

test saturating_add_i8 {
    assert(I8_MAX == saturating_add_i8(I8_MAX, 5));
    assert(I8_MIN == saturating_add_i8(I8_MIN, -5));
    assert(5 == saturating_add_i8(2, 3));
}

pub fn saturating_add_i16(a: i16, b: i16) i16 {
    return a +| b;
}

test saturating_add_i16 {
    assert(I16_MAX == saturating_add_i16(I16_MAX, 5));
    assert(I16_MIN == saturating_add_i16(I16_MIN, -5));
    assert(5 == saturating_add_i16(2, 3));
}

pub fn saturating_add_i32(a: i32, b: i32) i32 {
    return a +| b;
}

test saturating_add_i32 {
    assert(I32_MAX == saturating_add_i32(I32_MAX, 5));
    assert(I32_MIN == saturating_add_i32(I32_MIN, -5));
    assert(5 == saturating_add_i32(2, 3));
}

pub fn saturating_add_i64(a: i64, b: i64) i64 {
    return a +| b;
}

test saturating_add_i64 {
    assert(I64_MAX == saturating_add_i64(I64_MAX, 5));
    assert(I64_MIN == saturating_add_i64(I64_MIN, -5));
    assert(5 == saturating_add_i64(2, 3));
}

pub fn add_unsafe_u8(a: u8, b: u8) u8 {
    return a + b;
}

test add_unsafe_u8 {
    assert(8 == add_unsafe_u8(3, 5));
}

pub fn add_unsafe_u16(a: u16, b: u16) u16 {
    return a + b;
}

test add_unsafe_u16 {
    assert(8 == add_unsafe_u16(3, 5));
}

pub fn add_unsafe_u32(a: u32, b: u32) u32 {
    return a + b;
}

test add_unsafe_u32 {
    assert(8 == add_unsafe_u32(3, 5));
}

pub fn add_unsafe_u64(a: u64, b: u64) u64 {
    return a + b;
}

test add_unsafe_u64 {
    assert(8 == add_unsafe_u64(3, 5));
}

pub fn add_unsafe_i8(a: i8, b: i8) i8 {
    return a + b;
}

test add_unsafe_i8 {
    assert(8 == add_unsafe_i8(3, 5));
}

pub fn add_unsafe_i16(a: i16, b: i16) i16 {
    return a + b;
}

test add_unsafe_i16 {
    assert(8 == add_unsafe_i16(3, 5));
}

pub fn add_unsafe_i32(a: i32, b: i32) i32 {
    return a + b;
}

test add_unsafe_i32 {
    assert(8 == add_unsafe_i32(3, 5));
}

pub fn add_unsafe_i64(a: i64, b: i64) i64 {
    return a + b;
}

test add_unsafe_i64 {
    assert(8 == add_unsafe_i64(3, 5));
}

pub fn sub_u8(a: u8, b: u8) OverflowErr!u8 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_u8 {
    assert(OverflowErr.IntegerOverflow == sub_u8(0, 1));
    assert(U8_MAX == sub_u8(U8_MAX, 0) catch unreachable);
    assert(5 == sub_u8(10, 5) catch unreachable);
}

pub fn sub_u16(a: u16, b: u16) OverflowErr!u16 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_u16 {
    assert(OverflowErr.IntegerOverflow == sub_u16(0, 1));
    assert(U16_MAX == sub_u16(U16_MAX, 0) catch unreachable);
    assert(5 == sub_u16(10, 5) catch unreachable);
}

pub fn sub_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_u32 {
    assert(OverflowErr.IntegerOverflow == sub_u32(0, 1));
    assert(U32_MAX == sub_u32(U32_MAX, 0) catch unreachable);
    assert(5 == sub_u32(10, 5) catch unreachable);
}

pub fn sub_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_u64 {
    assert(OverflowErr.IntegerOverflow == sub_u64(0, 1));
    assert(U64_MAX == sub_u64(U64_MAX, 0) catch unreachable);
    assert(5 == sub_u64(10, 5) catch unreachable);
}

pub fn sub_i8(a: i8, b: i8) OverflowErr!i8 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_i8 {
    assert(OverflowErr.IntegerOverflow == sub_i8(I8_MIN, 1));
    assert(I8_MAX == sub_i8(I8_MAX, 0) catch unreachable);
    assert(5 == sub_i8(10, 5) catch unreachable);
}

pub fn sub_i16(a: i16, b: i16) OverflowErr!i16 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_i16 {
    assert(OverflowErr.IntegerOverflow == sub_i16(I16_MIN, 1));
    assert(I16_MAX == sub_i16(I16_MAX, 0) catch unreachable);
    assert(5 == sub_i16(10, 5) catch unreachable);
}

pub fn sub_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_i32 {
    assert(OverflowErr.IntegerOverflow == sub_i32(I32_MIN, 1));
    assert(I32_MAX == sub_i32(I32_MAX, 0) catch unreachable);
    assert(5 == sub_i32(10, 5) catch unreachable);
}

pub fn sub_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @subWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test sub_i64 {
    assert(OverflowErr.IntegerOverflow == sub_i64(I64_MIN, 1));
    assert(I64_MAX == sub_i64(I64_MAX, 0) catch unreachable);
    assert(5 == sub_i64(10, 5) catch unreachable);
}

pub fn wrapping_sub_u8(a: u8, b: u8) u8 {
    return a -% b;
}

test wrapping_sub_u8 {
    assert(U8_MAX == wrapping_sub_u8(0, 1));
    assert(5 == wrapping_sub_u8(10, 5));
}

pub fn wrapping_sub_u16(a: u16, b: u16) u16 {
    return a -% b;
}

test wrapping_sub_u16 {
    assert(U16_MAX == wrapping_sub_u16(0, 1));
    assert(5 == wrapping_sub_u16(10, 5));
}

pub fn wrapping_sub_u32(a: u32, b: u32) u32 {
    return a -% b;
}

test wrapping_sub_u32 {
    assert(U32_MAX == wrapping_sub_u32(0, 1));
    assert(5 == wrapping_sub_u32(10, 5));
}

pub fn wrapping_sub_u64(a: u64, b: u64) u64 {
    return a -% b;
}

test wrapping_sub_u64 {
    assert(U64_MAX == wrapping_sub_u64(0, 1));
    assert(5 == wrapping_sub_u64(10, 5));
}

pub fn saturating_sub_u8(a: u8, b: u8) u8 {
    return a -| b;
}

test saturating_sub_u8 {
    assert(0 == saturating_sub_u8(0, 5));
    assert(5 == saturating_sub_u8(10, 5));
}

pub fn saturating_sub_u16(a: u16, b: u16) u16 {
    return a -| b;
}

test saturating_sub_u16 {
    assert(0 == saturating_sub_u16(0, 5));
    assert(5 == saturating_sub_u16(10, 5));
}

pub fn saturating_sub_u32(a: u32, b: u32) u32 {
    return a -| b;
}

test saturating_sub_u32 {
    assert(0 == saturating_sub_u32(0, 5));
    assert(5 == saturating_sub_u32(10, 5));
}

pub fn saturating_sub_u64(a: u64, b: u64) u64 {
    return a -| b;
}

test saturating_sub_u64 {
    assert(0 == saturating_sub_u64(0, 5));
    assert(5 == saturating_sub_u64(10, 5));
}

pub fn saturating_sub_i8(a: i8, b: i8) i8 {
    return a -| b;
}

test saturating_sub_i8 {
    assert(I8_MIN == saturating_sub_i8(I8_MIN, 5));
    assert(I8_MAX == saturating_sub_i8(I8_MAX, -5));
    assert(5 == saturating_sub_i8(10, 5));
}

pub fn saturating_sub_i16(a: i16, b: i16) i16 {
    return a -| b;
}

test saturating_sub_i16 {
    assert(I16_MIN == saturating_sub_i16(I16_MIN, 5));
    assert(I16_MAX == saturating_sub_i16(I16_MAX, -5));
    assert(5 == saturating_sub_i16(10, 5));
}

pub fn saturating_sub_i32(a: i32, b: i32) i32 {
    return a -| b;
}

test saturating_sub_i32 {
    assert(I32_MIN == saturating_sub_i32(I32_MIN, 5));
    assert(I32_MAX == saturating_sub_i32(I32_MAX, -5));
    assert(5 == saturating_sub_i32(10, 5));
}

pub fn saturating_sub_i64(a: i64, b: i64) i64 {
    return a -| b;
}

test saturating_sub_i64 {
    assert(I64_MIN == saturating_sub_i64(I64_MIN, 5));
    assert(I64_MAX == saturating_sub_i64(I64_MAX, -5));
    assert(5 == saturating_sub_i64(10, 5));
}

pub fn sub_unsafe_u8(a: u8, b: u8) u8 {
    return a - b;
}

test sub_unsafe_u8 {
    assert(5 == sub_unsafe_u8(10, 5));
}

pub fn sub_unsafe_u16(a: u16, b: u16) u16 {
    return a - b;
}

test sub_unsafe_u16 {
    assert(5 == sub_unsafe_u16(10, 5));
}

pub fn sub_unsafe_u32(a: u32, b: u32) u32 {
    return a - b;
}

test sub_unsafe_u32 {
    assert(5 == sub_unsafe_u32(10, 5));
}

pub fn sub_unsafe_u64(a: u64, b: u64) u64 {
    return a - b;
}

test sub_unsafe_u64 {
    assert(5 == sub_unsafe_u64(10, 5));
}

pub fn sub_unsafe_i8(a: i8, b: i8) i8 {
    return a - b;
}

test sub_unsafe_i8 {
    assert(5 == sub_unsafe_i8(10, 5));
}

pub fn sub_unsafe_i16(a: i16, b: i16) i16 {
    return a - b;
}

test sub_unsafe_i16 {
    assert(5 == sub_unsafe_i16(10, 5));
}

pub fn sub_unsafe_i32(a: i32, b: i32) i32 {
    return a - b;
}

test sub_unsafe_i32 {
    assert(5 == sub_unsafe_i32(10, 5));
}

pub fn sub_unsafe_i64(a: i64, b: i64) i64 {
    return a - b;
}

test sub_unsafe_i64 {
    assert(5 == sub_unsafe_i64(10, 5));
}

pub fn mul_u8(a: u8, b: u8) OverflowErr!u8 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_u8 {
    assert(OverflowErr.IntegerOverflow == mul_u8(200, 2));
    assert(U8_MAX == mul_u8(U8_MAX, 1) catch unreachable);
    assert(12 == mul_u8(3, 4) catch unreachable);
}

pub fn mul_u16(a: u16, b: u16) OverflowErr!u16 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_u16 {
    assert(OverflowErr.IntegerOverflow == mul_u16(40000, 2));
    assert(U16_MAX == mul_u16(U16_MAX, 1) catch unreachable);
    assert(12 == mul_u16(3, 4) catch unreachable);
}

pub fn mul_u32(a: u32, b: u32) OverflowErr!u32 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_u32 {
    assert(OverflowErr.IntegerOverflow == mul_u32(2_000_000_000, 2));
    assert(U32_MAX == mul_u32(U32_MAX, 1) catch unreachable);
    assert(12 == mul_u32(3, 4) catch unreachable);
}

pub fn mul_u64(a: u64, b: u64) OverflowErr!u64 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_u64 {
    assert(OverflowErr.IntegerOverflow == mul_u64(4_000_000_000_000_000_000, 2));
    assert(U64_MAX == mul_u64(U64_MAX, 1) catch unreachable);
    assert(12 == mul_u64(3, 4) catch unreachable);
}

pub fn mul_i8(a: i8, b: i8) OverflowErr!i8 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_i8 {
    assert(OverflowErr.IntegerOverflow == mul_i8(40, 4));
    assert(I8_MAX == mul_i8(I8_MAX, 1) catch unreachable);
    assert(12 == mul_i8(3, 4) catch unreachable);
}

pub fn mul_i16(a: i16, b: i16) OverflowErr!i16 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_i16 {
    assert(OverflowErr.IntegerOverflow == mul_i16(20000, 2));
    assert(I16_MAX == mul_i16(I16_MAX, 1) catch unreachable);
    assert(12 == mul_i16(3, 4) catch unreachable);
}

pub fn mul_i32(a: i32, b: i32) OverflowErr!i32 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_i32 {
    assert(OverflowErr.IntegerOverflow == mul_i32(2_000_000_000, 2));
    assert(I32_MAX == mul_i32(I32_MAX, 1) catch unreachable);
    assert(12 == mul_i32(3, 4) catch unreachable);
}

pub fn mul_i64(a: i64, b: i64) OverflowErr!i64 {
    const res, const overflow = @mulWithOverflow(a, b);

    if (overflow == 1) {
        @branchHint(.cold);
        return OverflowErr.IntegerOverflow;
    }

    return res;
}

test mul_i64 {
    assert(OverflowErr.IntegerOverflow == mul_i64(4_000_000_000_000_000_000, 2));
    assert(I64_MAX == mul_i64(I64_MAX, 1) catch unreachable);
    assert(12 == mul_i64(3, 4) catch unreachable);
}

pub fn wrapping_mul_u8(a: u8, b: u8) u8 {
    return a *% b;
}

test wrapping_mul_u8 {
    assert(0 == wrapping_mul_u8(200, 2)); // wraps around
    assert(12 == wrapping_mul_u8(3, 4));
}

pub fn wrapping_mul_u16(a: u16, b: u16) u16 {
    return a *% b;
}

test wrapping_mul_u16 {
    assert(0 == wrapping_mul_u16(40000, 2));
    assert(12 == wrapping_mul_u16(3, 4));
}

pub fn wrapping_mul_u32(a: u32, b: u32) u32 {
    return a *% b;
}

test wrapping_mul_u32 {
    assert(0 == wrapping_mul_u32(2_000_000_000, 2));
    assert(12 == wrapping_mul_u32(3, 4));
}

pub fn wrapping_mul_u64(a: u64, b: u64) u64 {
    return a *% b;
}

test wrapping_mul_u64 {
    assert(0 == wrapping_mul_u64(4_000_000_000_000_000_000, 2));
    assert(12 == wrapping_mul_u64(3, 4));
}

pub fn saturating_mul_u8(a: u8, b: u8) u8 {
    return a *| b;
}

test saturating_mul_u8 {
    assert(U8_MAX == saturating_mul_u8(200, 2));
    assert(12 == saturating_mul_u8(3, 4));
}

pub fn saturating_mul_u16(a: u16, b: u16) u16 {
    return a *| b;
}

test saturating_mul_u16 {
    assert(U16_MAX == saturating_mul_u16(40000, 2));
    assert(12 == saturating_mul_u16(3, 4));
}

pub fn saturating_mul_u32(a: u32, b: u32) u32 {
    return a *| b;
}

test saturating_mul_u32 {
    assert(U32_MAX == saturating_mul_u32(2_000_000_000, 2));
    assert(12 == saturating_mul_u32(3, 4));
}

pub fn saturating_mul_u64(a: u64, b: u64) u64 {
    return a *| b;
}

test saturating_mul_u64 {
    assert(U64_MAX == saturating_mul_u64(4_000_000_000_000_000_000, 2));
    assert(12 == saturating_mul_u64(3, 4));
}

pub fn mul_unsafe_u8(a: u8, b: u8) u8 {
    return a * b;
}

test mul_unsafe_u8 {
    assert(12 == mul_unsafe_u8(3, 4));
}

pub fn mul_unsafe_u16(a: u16, b: u16) u16 {
    return a * b;
}

test mul_unsafe_u16 {
    assert(12 == mul_unsafe_u16(3, 4));
}

pub fn mul_unsafe_u32(a: u32, b: u32) u32 {
    return a * b;
}

test mul_unsafe_u32 {
    assert(12 == mul_unsafe_u32(3, 4));
}

pub fn mul_unsafe_u64(a: u64, b: u64) u64 {
    return a * b;
}

test mul_unsafe_u64 {
    assert(12 == mul_unsafe_u64(3, 4));
}

pub fn mul_unsafe_i8(a: i8, b: i8) i8 {
    return a * b;
}

test mul_unsafe_i8 {
    assert(12 == mul_unsafe_i8(3, 4));
}

pub fn mul_unsafe_i16(a: i16, b: i16) i16 {
    return a * b;
}

test mul_unsafe_i16 {
    assert(12 == mul_unsafe_i16(3, 4));
}

pub fn mul_unsafe_i32(a: i32, b: i32) i32 {
    return a * b;
}

test mul_unsafe_i32 {
    assert(12 == mul_unsafe_i32(3, 4));
}

pub fn mul_unsafe_i64(a: i64, b: i64) i64 {
    return a * b;
}

test mul_unsafe_i64 {
    assert(12 == mul_unsafe_i64(3, 4));
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

test div_u8 {
    assert(DivErr.DivisionByZero == div_u8(10, 0));
    assert(5 == div_u8(10, 2) catch unreachable);
    assert(0 == div_u8(0, 5) catch unreachable);
}

pub fn div_u16(a: u16, b: u16) DivErr!u16 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

test div_u16 {
    assert(DivErr.DivisionByZero == div_u16(10, 0));
    assert(5 == div_u16(10, 2) catch unreachable);
    assert(0 == div_u16(0, 5) catch unreachable);
}

pub fn div_u32(a: u32, b: u32) DivErr!u32 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

test div_u32 {
    assert(DivErr.DivisionByZero == div_u32(10, 0));
    assert(5 == div_u32(10, 2) catch unreachable);
    assert(0 == div_u32(0, 5) catch unreachable);
}

pub fn div_u64(a: u64, b: u64) DivErr!u64 {
    if (b == 0) {
        @branchHint(.cold);
        return DivErr.DivisionByZero;
    }
    return a / b;
}

test div_u64 {
    assert(DivErr.DivisionByZero == div_u64(10, 0));
    assert(5 == div_u64(10, 2) catch unreachable);
    assert(0 == div_u64(0, 5) catch unreachable);
}

pub fn div_unsafe_u8(a: u8, b: u8) u8 {
    return a / b;
}

test div_unsafe_u8 {
    assert(5 == div_unsafe_u8(10, 2));
}

pub fn div_unsafe_u16(a: u16, b: u16) u16 {
    return a / b;
}

test div_unsafe_u16 {
    assert(5 == div_unsafe_u16(10, 2));
}

pub fn div_unsafe_u32(a: u32, b: u32) u32 {
    return a / b;
}

test div_unsafe_u32 {
    assert(5 == div_unsafe_u32(10, 2));
}

pub fn div_unsafe_u64(a: u64, b: u64) u64 {
    return a / b;
}

test div_unsafe_u64 {
    assert(5 == div_unsafe_u64(10, 2));
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

test mod_u8 {
    assert(ModErr.DivisionByZero == mod_u8(10, 0));
    assert(1 == mod_u8(10, 3) catch unreachable);
    assert(0 == mod_u8(0, 5) catch unreachable);
}

pub fn mod_u16(a: u16, b: u16) ModErr!u16 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

test mod_u16 {
    assert(ModErr.DivisionByZero == mod_u16(10, 0));
    assert(1 == mod_u16(10, 3) catch unreachable);
    assert(0 == mod_u16(0, 5) catch unreachable);
}

pub fn mod_u32(a: u32, b: u32) ModErr!u32 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

test mod_u32 {
    assert(ModErr.DivisionByZero == mod_u32(10, 0));
    assert(1 == mod_u32(10, 3) catch unreachable);
    assert(0 == mod_u32(0, 5) catch unreachable);
}

pub fn mod_u64(a: u64, b: u64) ModErr!u64 {
    if (b == 0) {
        @branchHint(.cold);
        return ModErr.DivisionByZero;
    }
    return a % b;
}

test mod_u64 {
    assert(ModErr.DivisionByZero == mod_u64(10, 0));
    assert(1 == mod_u64(10, 3) catch unreachable);
    assert(0 == mod_u64(0, 5) catch unreachable);
}

pub fn mod_unsafe_u8(a: u8, b: u8) u8 {
    return a % b;
}

test mod_unsafe_u8 {
    assert(1 == mod_unsafe_u8(10, 3));
}

pub fn mod_unsafe_u16(a: u16, b: u16) u16 {
    return a % b;
}

test mod_unsafe_u16 {
    assert(1 == mod_unsafe_u16(10, 3));
}

pub fn mod_unsafe_u32(a: u32, b: u32) u32 {
    return a % b;
}

test mod_unsafe_u32 {
    assert(1 == mod_unsafe_u32(10, 3));
}

pub fn mod_unsafe_u64(a: u64, b: u64) u64 {
    return a % b;
}

test mod_unsafe_u64 {
    assert(1 == mod_unsafe_u64(10, 3));
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

test div_ceil_u8 {
    assert(DivCeilErr.DivisionByZero == div_ceil_u8(10, 0));
    assert(DivCeilErr.IntegerOverflow == div_ceil_u8(U8_MAX, 2));
    assert(3 == div_ceil_u8(7, 2) catch unreachable);
    assert(2 == div_ceil_u8(4, 2) catch unreachable);
}

pub fn div_ceil_u16(a: u16, b: u16) DivCeilErr!u16 {
    const c = try add_u16(a, b);
    const d = try sub_u16(c, 1);
    return try div_u16(d, b);
}

test div_ceil_u16 {
    assert(DivCeilErr.DivisionByZero == div_ceil_u16(10, 0));
    assert(DivCeilErr.IntegerOverflow == div_ceil_u16(U16_MAX, 2));
    assert(3 == div_ceil_u16(7, 2) catch unreachable);
    assert(2 == div_ceil_u16(4, 2) catch unreachable);
}

pub fn div_ceil_u32(a: u32, b: u32) DivCeilErr!u32 {
    const c = try add_u32(a, b);
    const d = try sub_u32(c, 1);
    return try div_u32(d, b);
}

test div_ceil_u32 {
    assert(DivCeilErr.DivisionByZero == div_ceil_u32(10, 0));
    assert(DivCeilErr.IntegerOverflow == div_ceil_u32(U32_MAX, 2));
    assert(3 == div_ceil_u32(7, 2) catch unreachable);
    assert(2 == div_ceil_u32(4, 2) catch unreachable);
}

pub fn div_ceil_u64(a: u64, b: u64) DivCeilErr!u64 {
    const c = try add_u64(a, b);
    const d = try sub_u64(c, 1);
    return try div_u64(d, b);
}

test div_ceil_u64 {
    assert(DivCeilErr.DivisionByZero == div_ceil_u64(10, 0));
    assert(DivCeilErr.IntegerOverflow == div_ceil_u64(U64_MAX, 2));
    assert(3 == div_ceil_u64(7, 2) catch unreachable);
    assert(2 == div_ceil_u64(4, 2) catch unreachable);
}

pub fn div_ceil_unsafe_u8(a: u8, b: u8) u8 {
    const c = add_unsafe_u8(a, b);
    const d = sub_unsafe_u8(c, 1);
    return div_unsafe_u8(d, b);
}

test div_ceil_unsafe_u8 {
    assert(3 == div_ceil_unsafe_u8(7, 2));
    assert(2 == div_ceil_unsafe_u8(4, 2));
}

pub fn div_ceil_unsafe_u16(a: u16, b: u16) u16 {
    const c = add_unsafe_u16(a, b);
    const d = sub_unsafe_u16(c, 1);
    return div_unsafe_u16(d, b);
}

test div_ceil_unsafe_u16 {
    assert(3 == div_ceil_unsafe_u16(7, 2));
    assert(2 == div_ceil_unsafe_u16(4, 2));
}

pub fn div_ceil_unsafe_u32(a: u32, b: u32) u32 {
    const c = add_unsafe_u32(a, b);
    const d = sub_unsafe_u32(c, 1);
    return div_unsafe_u32(d, b);
}

test div_ceil_unsafe_u32 {
    assert(3 == div_ceil_unsafe_u32(7, 2));
    assert(2 == div_ceil_unsafe_u32(4, 2));
}

pub fn div_ceil_unsafe_u64(a: u64, b: u64) u64 {
    const c = add_unsafe_u64(a, b);
    const d = sub_unsafe_u64(c, 1);
    return div_unsafe_u64(d, b);
}

test div_ceil_unsafe_u64 {
    assert(3 == div_ceil_unsafe_u64(7, 2));
    assert(2 == div_ceil_unsafe_u64(4, 2));
}

pub const NextMulOfErr = error{
    IntegerOverflow,
    DivisionByZero,
};

pub fn next_multiple_of_u8(a: u8, b: u8) NextMulOfErr!u8 {
    const c = try div_ceil_u8(a, b);
    return try mul_u8(c, b);
}

test next_multiple_of_u8 {
    assert(NextMulOfErr.DivisionByZero == next_multiple_of_u8(10, 0));
    assert(NextMulOfErr.IntegerOverflow == next_multiple_of_u8(U8_MAX, 2));
    assert(10 == next_multiple_of_u8(7, 2) catch unreachable);
    assert(8 == next_multiple_of_u8(4, 2) catch unreachable);
}

pub fn next_multiple_of_u16(a: u16, b: u16) NextMulOfErr!u16 {
    const c = try div_ceil_u16(a, b);
    return try mul_u16(c, b);
}

test next_multiple_of_u16 {
    assert(NextMulOfErr.DivisionByZero == next_multiple_of_u16(10, 0));
    assert(NextMulOfErr.IntegerOverflow == next_multiple_of_u16(U16_MAX, 2));
    assert(10 == next_multiple_of_u16(7, 2) catch unreachable);
    assert(8 == next_multiple_of_u16(4, 2) catch unreachable);
}

pub fn next_multiple_of_u32(a: u32, b: u32) NextMulOfErr!u32 {
    const c = try div_ceil_u32(a, b);
    return try mul_u32(c, b);
}

test next_multiple_of_u32 {
    assert(NextMulOfErr.DivisionByZero == next_multiple_of_u32(10, 0));
    assert(NextMulOfErr.IntegerOverflow == next_multiple_of_u32(U32_MAX, 2));
    assert(10 == next_multiple_of_u32(7, 2) catch unreachable);
    assert(8 == next_multiple_of_u32(4, 2) catch unreachable);
}

pub fn next_multiple_of_u64(a: u64, b: u64) NextMulOfErr!u64 {
    const c = try div_ceil_u64(a, b);
    return try mul_u64(c, b);
}

test next_multiple_of_u64 {
    assert(NextMulOfErr.DivisionByZero == next_multiple_of_u64(10, 0));
    assert(NextMulOfErr.IntegerOverflow == next_multiple_of_u64(U64_MAX, 2));
    assert(10 == next_multiple_of_u64(7, 2) catch unreachable);
    assert(8 == next_multiple_of_u64(4, 2) catch unreachable);
}

pub fn next_multiple_of_unsafe_u8(a: u8, b: u8) u8 {
    const c = div_ceil_unsafe_u8(a, b);
    return mul_unsafe_u8(c, b);
}

test next_multiple_of_unsafe_u8 {
    assert(10 == next_multiple_of_unsafe_u8(7, 2));
    assert(8 == next_multiple_of_unsafe_u8(4, 2));
}

pub fn next_multiple_of_unsafe_u16(a: u16, b: u16) u16 {
    const c = div_ceil_unsafe_u16(a, b);
    return mul_unsafe_u16(c, b);
}

test next_multiple_of_unsafe_u16 {
    assert(10 == next_multiple_of_unsafe_u16(7, 2));
    assert(8 == next_multiple_of_unsafe_u16(4, 2));
}

pub fn next_multiple_of_unsafe_u32(a: u32, b: u32) u32 {
    const c = div_ceil_unsafe_u32(a, b);
    return mul_unsafe_u32(c, b);
}

test next_multiple_of_unsafe_u32 {
    assert(10 == next_multiple_of_unsafe_u32(7, 2));
    assert(8 == next_multiple_of_unsafe_u32(4, 2));
}

pub fn next_multiple_of_unsafe_u64(a: u64, b: u64) u64 {
    const c = div_ceil_unsafe_u64(a, b);
    return mul_unsafe_u64(c, b);
}

test next_multiple_of_unsafe_u64 {
    assert(10 == next_multiple_of_unsafe_u64(7, 2));
    assert(8 == next_multiple_of_unsafe_u64(4, 2));
}

pub fn min_u8(a: u8, b: u8) u8 {
    return @min(a, b);
}

test min_u8 {
    assert(3 == min_u8(3, 5));
    assert(5 == min_u8(5, 3));
    assert(5 == min_u8(5, 5));
}

pub fn min_u16(a: u16, b: u16) u16 {
    return @min(a, b);
}

test min_u16 {
    assert(3 == min_u16(3, 5));
    assert(5 == min_u16(5, 3));
    assert(5 == min_u16(5, 5));
}

pub fn min_u32(a: u32, b: u32) u32 {
    return @min(a, b);
}

test min_u32 {
    assert(3 == min_u32(3, 5));
    assert(5 == min_u32(5, 3));
    assert(5 == min_u32(5, 5));
}

pub fn min_u64(a: u64, b: u64) u64 {
    return @min(a, b);
}

test min_u64 {
    assert(3 == min_u64(3, 5));
    assert(5 == min_u64(5, 3));
    assert(5 == min_u64(5, 5));
}

pub fn min_i8(a: i8, b: i8) i8 {
    return @min(a, b);
}

test min_i8 {
    assert(-5 == min_i8(-5, 3));
    assert(3 == min_i8(3, -5));
    assert(5 == min_i8(5, 5));
}

pub fn min_i16(a: i16, b: i16) i16 {
    return @min(a, b);
}

test min_i16 {
    assert(-5 == min_i16(-5, 3));
    assert(3 == min_i16(3, -5));
    assert(5 == min_i16(5, 5));
}

pub fn min_i32(a: i32, b: i32) i32 {
    return @min(a, b);
}

test min_i32 {
    assert(-5 == min_i32(-5, 3));
    assert(3 == min_i32(3, -5));
    assert(5 == min_i32(5, 5));
}

pub fn min_i64(a: i64, b: i64) i64 {
    return @min(a, b);
}

test min_i64 {
    assert(-5 == min_i64(-5, 3));
    assert(3 == min_i64(3, -5));
    assert(5 == min_i64(5, 5));
}

pub fn max_u8(a: u8, b: u8) u8 {
    return @max(a, b);
}

test max_u8 {
    assert(5 == max_u8(3, 5));
    assert(5 == max_u8(5, 3));
    assert(5 == max_u8(5, 5));
}

pub fn max_u16(a: u16, b: u16) u16 {
    return @max(a, b);
}

test max_u16 {
    assert(5 == max_u16(3, 5));
    assert(5 == max_u16(5, 3));
    assert(5 == max_u16(5, 5));
}

pub fn max_u32(a: u32, b: u32) u32 {
    return @max(a, b);
}

test max_u32 {
    assert(5 == max_u32(3, 5));
    assert(5 == max_u32(5, 3));
    assert(5 == max_u32(5, 5));
}

pub fn max_u64(a: u64, b: u64) u64 {
    return @max(a, b);
}

test max_u64 {
    assert(5 == max_u64(3, 5));
    assert(5 == max_u64(5, 3));
    assert(5 == max_u64(5, 5));
}

pub fn max_i8(a: i8, b: i8) i8 {
    return @max(a, b);
}

test max_i8 {
    assert(3 == max_i8(-5, 3));
    assert(3 == max_i8(3, -5));
    assert(5 == max_i8(5, 5));
}

pub fn max_i16(a: i16, b: i16) i16 {
    return @max(a, b);
}

test max_i16 {
    assert(3 == max_i16(-5, 3));
    assert(3 == max_i16(3, -5));
    assert(5 == max_i16(5, 5));
}

pub fn max_i32(a: i32, b: i32) i32 {
    return @max(a, b);
}

test max_i32 {
    assert(3 == max_i32(-5, 3));
    assert(3 == max_i32(3, -5));
    assert(5 == max_i32(5, 5));
}

pub fn max_i64(a: i64, b: i64) i64 {
    return @max(a, b);
}

test max_i64 {
    assert(3 == max_i64(-5, 3));
    assert(3 == max_i64(3, -5));
    assert(5 == max_i64(5, 5));
}

pub fn ctz_u8(a: u8) u8 {
    return @ctz(a);
}

test ctz_u8 {
    assert(0 == ctz_u8(1));
    assert(2 == ctz_u8(4));
    assert(7 == ctz_u8(128));
}

pub fn ctz_u16(a: u16) u8 {
    return @ctz(a);
}

test ctz_u16 {
    assert(0 == ctz_u16(1));
    assert(2 == ctz_u16(4));
    assert(7 == ctz_u16(128));
}

pub fn ctz_u32(a: u32) u8 {
    return @ctz(a);
}

test ctz_u32 {
    assert(0 == ctz_u32(1));
    assert(2 == ctz_u32(4));
    assert(7 == ctz_u32(128));
}

pub fn ctz_u64(a: u64) u8 {
    return @ctz(a);
}

test ctz_u64 {
    assert(0 == ctz_u64(1));
    assert(2 == ctz_u64(4));
    assert(7 == ctz_u64(128));
}

pub fn clz_u8(a: u8) u8 {
    return @clz(a);
}

test clz_u8 {
    assert(0 == clz_u8(U8_MAX));
    assert(2 == clz_u8(63));
    assert(7 == clz_u8(1));
}

pub fn clz_u16(a: u16) u8 {
    return @clz(a);
}

test clz_u16 {
    assert(0 == clz_u16(U16_MAX));
    assert(2 == clz_u16(16383));
    assert(15 == clz_u16(1));
}

pub fn clz_u32(a: u32) u8 {
    return @clz(a);
}

test clz_u32 {
    assert(0 == clz_u32(U32_MAX));
    assert(2 == clz_u32(1_073_741_823));
    assert(31 == clz_u32(1));
}

pub fn clz_u64(a: u64) u8 {
    return @clz(a);
}

test clz_u64 {
    assert(0 == clz_u64(U64_MAX));
    assert(2 == clz_u64(274_877_906_943));
    assert(63 == clz_u64(1));
}

pub fn popcnt_u8(a: u8) u8 {
    return @popCount(a);
}

test popcnt_u8 {
    assert(0 == popcnt_u8(0));
    assert(1 == popcnt_u8(1));
    assert(8 == popcnt_u8(U8_MAX));
}

pub fn popcnt_u16(a: u16) u8 {
    return @popCount(a);
}

test popcnt_u16 {
    assert(0 == popcnt_u16(0));
    assert(1 == popcnt_u16(1));
    assert(16 == popcnt_u16(U16_MAX));
}

pub fn popcnt_u32(a: u32) u8 {
    return @popCount(a);
}

test popcnt_u32 {
    assert(0 == popcnt_u32(0));
    assert(1 == popcnt_u32(1));
    assert(32 == popcnt_u32(U32_MAX));
}

pub fn popcnt_u64(a: u64) u8 {
    return @popCount(a);
}

test popcnt_u64 {
    assert(0 == popcnt_u64(0));
    assert(1 == popcnt_u64(1));
    assert(64 == popcnt_u64(U64_MAX));
}

pub fn bit_width_u8(a: u8) u8 {
    return 8 - @clz(a);
}

test bit_width_u8 {
    assert(0 == bit_width_u8(0));
    assert(1 == bit_width_u8(1));
    assert(8 == bit_width_u8(U8_MAX));
    assert(4 == bit_width_u8(8));
}

pub fn bit_width_u16(a: u16) u8 {
    return 16 - @clz(a);
}

test bit_width_u16 {
    assert(0 == bit_width_u16(0));
    assert(1 == bit_width_u16(1));
    assert(16 == bit_width_u16(U16_MAX));
    assert(4 == bit_width_u16(8));
}

pub fn bit_width_u32(a: u32) u8 {
    return 32 - @clz(a);
}

test bit_width_u32 {
    assert(0 == bit_width_u32(0));
    assert(1 == bit_width_u32(1));
    assert(32 == bit_width_u32(U32_MAX));
    assert(4 == bit_width_u32(8));
}

pub fn bit_width_u64(a: u64) u8 {
    return 64 - @clz(a);
}

test bit_width_u64 {
    assert(0 == bit_width_u64(0));
    assert(1 == bit_width_u64(1));
    assert(64 == bit_width_u64(U64_MAX));
    assert(4 == bit_width_u64(8));
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

test shl_u8 {
    assert(ShiftErr.IllegalShift == shl_u8(1, 8));
    assert(1 == shl_u8(1, 0) catch unreachable);
    assert(8 == shl_u8(1, 3) catch unreachable);
}

pub fn shl_u16(a: u16, amount: u64) ShiftErr!u16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_u16 {
    assert(ShiftErr.IllegalShift == shl_u16(1, 16));
    assert(1 == shl_u16(1, 0) catch unreachable);
    assert(8 == shl_u16(1, 3) catch unreachable);
}

pub fn shl_u32(a: u32, amount: u64) ShiftErr!u32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_u32 {
    assert(ShiftErr.IllegalShift == shl_u32(1, 32));
    assert(1 == shl_u32(1, 0) catch unreachable);
    assert(8 == shl_u32(1, 3) catch unreachable);
}

pub fn shl_u64(a: u64, amount: u64) ShiftErr!u64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_u64 {
    assert(ShiftErr.IllegalShift == shl_u64(1, 64));
    assert(1 == shl_u64(1, 0) catch unreachable);
    assert(8 == shl_u64(1, 3) catch unreachable);
}

pub fn shl_i8(a: i8, amount: u64) ShiftErr!i8 {
    if (amount >= 8) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_i8 {
    assert(ShiftErr.IllegalShift == shl_i8(1, 8));
    assert(1 == shl_i8(1, 0) catch unreachable);
    assert(8 == shl_i8(1, 3) catch unreachable);
}

pub fn shl_i16(a: i16, amount: u64) ShiftErr!i16 {
    if (amount >= 16) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_i16 {
    assert(ShiftErr.IllegalShift == shl_i16(1, 16));
    assert(1 == shl_i16(1, 0) catch unreachable);
    assert(8 == shl_i16(1, 3) catch unreachable);
}

pub fn shl_i32(a: i32, amount: u64) ShiftErr!i32 {
    if (amount >= 32) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_i32 {
    assert(ShiftErr.IllegalShift == shl_i32(1, 32));
    assert(1 == shl_i32(1, 0) catch unreachable);
    assert(8 == shl_i32(1, 3) catch unreachable);
}

pub fn shl_i64(a: i64, amount: u64) ShiftErr!i64 {
    if (amount >= 64) {
        @branchHint(.cold);
        return ShiftErr.IllegalShift;
    }
    return a << @truncate(amount);
}

test shl_i64 {
    assert(ShiftErr.IllegalShift == shl_i64(1, 64));
    assert(1 == shl_i64(1, 0) catch unreachable);
    assert(8 == shl_i64(1, 3) catch unreachable);
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

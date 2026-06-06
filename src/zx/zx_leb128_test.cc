#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_leb128.h>
#include <zx/zx_slice.h>

static void testcase_unsigned(zx_u8 const *buf, zx_u64 buf_len, zx_u64 num) {
  ZXSlice<zx_u8> slice = ZXSlice<zx_u8>::init_unsafe(buf, buf_len);

  zx_u64 out_num = ZX_U64_MAX;
  zx_u64 offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_uleb128_to_u64(slice, &offset, &out_num));
  zx_assert_always(offset == buf_len);
  zx_assert_always(out_num == num);

  zx_u8 out_buf[16];
  constexpr zx_u64 OUT_BUF_SIZE = sizeof(out_buf) / sizeof(out_buf[0]);

  ZXSliceMut<zx_u8> out_slice =
      ZXSliceMut<zx_u8>::init_unsafe(out_buf, OUT_BUF_SIZE);
  offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_uleb128_from_u64(num, out_slice, &offset));
  zx_assert_always(offset != buf_len);
  zx_assert_always(out_slice.slice_unsafe(0, offset).as_const().equals(slice));
}

static void testcase_signed(zx_u8 const *buf, zx_u64 buf_len, zx_i64 num) {
  ZXSlice<zx_u8> slice = ZXSlice<zx_u8>::init_unsafe(buf, buf_len);

  zx_i64 out_num = ZX_I64_MAX;
  zx_u64 offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_sleb128_to_i64(slice, &offset, &out_num));
  zx_assert_always(offset == buf_len);
  zx_assert_always(out_num == num);

  zx_u8 out_buf[16];
  constexpr zx_u64 OUT_BUF_SIZE = sizeof(out_buf) / sizeof(out_buf[0]);

  ZXSliceMut<zx_u8> out_slice =
      ZXSliceMut<zx_u8>::init_unsafe(out_buf, OUT_BUF_SIZE);
  offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_sleb128_from_i64(num, out_slice, &offset));
  zx_assert_always(offset == buf_len);
  zx_assert_always(out_slice.slice_unsafe(0, offset).as_const().equals(slice));
}

static void test_unsigned() {
  testcase_unsigned((zx_u8[]){2__wbu}, 1, 2);
  testcase_unsigned((zx_u8[]){127__wbu}, 1, 127);
  testcase_unsigned((zx_u8[]){0x80__wbu, 1__wbu}, 2, 128);
  testcase_unsigned((zx_u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2, 129);
  testcase_unsigned((zx_u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2, 129);
  testcase_unsigned((zx_u8[]){57__wbu | 0x80__wbu, 100__wbu}, 2, 12857__wbu);
}

static void test_signed() {
  testcase_signed((zx_u8[]){2__wbu}, 1, 2);
  testcase_signed((zx_u8[]){0x7e__wbu}, 1, -2);
  testcase_signed((zx_u8[]){127__wbu | 0x80__wbu, 0}, 2, 127);
  testcase_signed((zx_u8[]){1__wbu | 0x80__wbu, 0x7f__wbu}, 2, -127);
  testcase_signed((zx_u8[]){0x80__wbu, 1__wbu}, 2, 128);
  testcase_signed((zx_u8[]){0x80__wbu, 0x7f__wbu}, 2, -128);
  testcase_signed((zx_u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2, 129);
  testcase_signed((zx_u8[]){0x7f__wbu | 0x80__wbu, 0x7e__wbu}, 2, -129);
}

static void signed_roundtrip(ZXSliceMut<zx_u8> slice, zx_i64 i) {
  zx_u64 write_offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_sleb128_from_i64(i, slice, &write_offset));

  zx_u64 read_offset = 0;
  zx_i64 num = ZX_I64_MAX;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_sleb128_to_i64(slice.as_const(), &read_offset, &num));

  zx_assert_always(num == i);
  zx_assert_always(read_offset == write_offset);
}

static void unsigned_roundtrip(ZXSliceMut<zx_u8> slice, zx_u64 i) {
  zx_u64 write_offset = 0;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_uleb128_from_u64(i, slice, &write_offset));

  zx_u64 read_offset = 0;
  zx_u64 num = ZX_U64_MAX;
  zx_assert_always(ZXLeb128Status::Ok ==
                   zx_uleb128_to_u64(slice.as_const(), &read_offset, &num));

  zx_assert_always(num == i);
  zx_assert_always(read_offset == write_offset);
}

static void test_limits() {
  zx_u8 buf[15];
  constexpr zx_u64 SIZE = sizeof(buf) / sizeof(buf[0]);
  ZXSliceMut<zx_u8> slice = ZXSliceMut<zx_u8>::init_unsafe(buf, SIZE);

  unsigned_roundtrip(slice, ZX_U64_MAX);
  unsigned_roundtrip(slice, 0);
  unsigned_roundtrip(slice, 1);

  signed_roundtrip(slice, ZX_I64_MAX);
  signed_roundtrip(slice, ZX_I64_MIN);
  signed_roundtrip(slice, -1);
  signed_roundtrip(slice, 1);
}

static void test_roundtrip() {
  zx_u8 buf[15];
  constexpr zx_u64 SIZE = sizeof(buf) / sizeof(buf[0]);
  ZXSliceMut<zx_u8> slice = ZXSliceMut<zx_u8>::init_unsafe(buf, SIZE);

  for (zx_u64 i = 0; i < 1025; i++) {
    unsigned_roundtrip(slice, i);
  }

  for (zx_i64 i = -1025; i < 1024; i++) {
    signed_roundtrip(slice, i);
  }
}

static void test_read_with_overflow() {
  zx_u8 buf[] = {
      0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
      0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
      0xff__wbu, 0xff__wbu, 0b01111111__wbu, 0b11100100__wbu,
  };
  constexpr zx_u64 SIZE = sizeof(buf) / sizeof(buf[0]);

  ZXSlice<zx_u8> slice = ZXSlice<zx_u8>::init_unsafe(buf, SIZE);

  zx_u64 offset = 0;
  zx_u64 num = 0;
  zx_assert_always(ZXLeb128Status::InvalidInput ==
                   zx_uleb128_to_u64(slice, &offset, &num));

  offset = 0;
  zx_i64 inum = 0;
  zx_assert_always(ZXLeb128Status::InvalidInput ==
                   zx_sleb128_to_i64(slice, &offset, &inum));
}

int main(void) {
  test_unsigned();
  test_signed();
  test_limits();
  test_read_with_overflow();
  test_roundtrip();

  return 0;
}

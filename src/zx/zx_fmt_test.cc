#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

static void assert(bool v) {
  if (!v) {
    __builtin_trap();
  }
}

int main(void) {
  constexpr zx_u64 cap = 1 << 12;
  zx_u8 arr[cap];
  ZXSliceMut<zx_u8> mem = ZXSliceMut<zx_u8>::init_unsafe(arr, cap);
  ZXFmt fmt = ZXFmt::init(mem);
  bool overflow = false;
  fmt.push_i64(-1212, &overflow);
  assert(fmt.n_written() == 5);
  assert(!overflow);
  assert(ZXCString::init_unsafe("-1212").slice().equals(fmt.written()));

  fmt.push_char('x', &overflow);
  assert(fmt.n_written() == 6);
  assert(!overflow);
  assert(ZXCString::init_unsafe("-1212x").slice().equals(fmt.written()));

  fmt.push_i64(10, &overflow);
  assert(fmt.n_written() == 8);
  assert(!overflow);
  assert(ZXCString::init_unsafe("-1212x10").slice().equals(fmt.written()));

  fmt.push_u64(1010, &overflow);
  assert(fmt.n_written() == 12);
  assert(!overflow);
  assert(ZXCString::init_unsafe("-1212x101010").slice().equals(fmt.written()));

  ZXFmt ofmt = ZXFmt::init(ZXSliceMut<zx_u8>::init_dangling());
  bool oflow = false;
  ofmt.push_u64(1, &oflow);
  assert(oflow);

  return 0;
}

// zx_u64 n_written = 0;
// ZXSysStatus const w_status =
//     zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
// (void)w_status;
// (void)n_written;

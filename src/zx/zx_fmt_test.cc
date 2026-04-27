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
  // zx_u64 n_written = 0;
  // ZXSysStatus const w_status =
  //     zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
  // (void)w_status;
  // (void)n_written;

  return 0;
}

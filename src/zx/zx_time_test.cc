#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_time.h>

int main(void) {
  zx_u8 fmt_mem[1 << 12];
  __builtin_memset(fmt_mem, 0, sizeof(fmt_mem));
  ZXSliceMut<zx_u8> fmt_slice =
      ZXSliceMut<zx_u8>::init_unsafe(fmt_mem, sizeof(fmt_mem));
  ZXFmt fmt = ZXFmt::init(fmt_slice);

  ZXTimeInfo info;
  ZXTimeStatus const status = ZXTimeInfo::init(&info);
  if (status != ZXTimeStatus::Ok) {
    bool fmt_overflow = false;
    fmt.push_cstring(zx_time_status_to_cstring(status), &fmt_overflow);
    fmt.push_char('\n', &fmt_overflow);
    (void)fmt_overflow;
    zx_u64 n_written = 0;
    ZXSysStatus const w_status =
        zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
    (void)w_status;
    (void)n_written;
    __builtin_trap();
  }

  ZXTimer timer = ZXTimer::init(info);

  zx_u64 const l1 = timer.lap();
  zx_u64 const l2 = timer.lap();

  (void)l1;
  (void)l2;

  return 0;
}

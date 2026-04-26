#include <zx/zx_int.h>
#include <zx/zx_time.h>

int main(void) {
  ZXTimeInfo info;
  ZXTimeStatus const status = ZXTimeInfo::init(&info);
  if (status != ZXTimeStatus::Ok) {
    __builtin_trap();
  }

  ZXTimer timer = ZXTimer::init(info);

  zx_u64 const l1 = timer.lap();
  zx_u64 const l2 = timer.lap();

  (void)l1;
  (void)l2;

  return 0;
}

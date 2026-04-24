#include <zx/zx_int.h>
#include <zx/zx_time.h>

int main(void) {
  ZXTimer timer = ZXTimer::init();

  zx_u64 const l1 = timer.lap();
  zx_u64 const l2 = timer.lap();

  (void)l1;
  (void)l2;

  return 0;
}

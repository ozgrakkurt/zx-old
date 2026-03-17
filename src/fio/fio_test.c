#include "./fio_uring.h"
#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  FioUringSys poll_ring;
  FioUringSys ring;

  for (zx_u32 i = 0; i < 500; i++) {
    ZX_ERR_TRY_FATAL((FioUringSys_init(true, -1, &poll_ring, &err_stack)),
                     &err_stack);
    ZX_ERR_TRY_FATAL(
        (FioUringSys_init(false, poll_ring.ring_fd, &ring, &err_stack)),
        &err_stack);
    ZX_ERR_TRY_FATAL((FioUringSys_deinit(&ring, &err_stack)), &err_stack);
    ZX_ERR_TRY_FATAL((FioUringSys_deinit(&poll_ring, &err_stack)), &err_stack);
  }

  return 0;
}

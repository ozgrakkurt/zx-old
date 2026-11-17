#include <fio/fio_uring.h>
#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  FioUring poll_ring;
  FioUring ring;

  for (zx_u32 i = 0; i < 5; i++) {
    ZX_ERR_TRY_FATAL((FioUring_init(
                         &(FioUringParams){
                             .wq_fd = 0,
                             .polled_io = true,
                             .io_buf = nullptr,
                             .io_buf_size = 0,
                         },
                         &poll_ring, &err_stack)),
                     &err_stack);
    ZX_ERR_TRY_FATAL((FioUring_init(
                         &(FioUringParams){
                             .wq_fd = (zx_u32)poll_ring.ring_fd,
                             .polled_io = false,
                             .io_buf = nullptr,
                             .io_buf_size = 0,
                         },
                         &ring, &err_stack)),
                     &err_stack);
    ZX_ERR_TRY_FATAL((FioUring_deinit(&ring, &err_stack)), &err_stack);
    ZX_ERR_TRY_FATAL((FioUring_deinit(&poll_ring, &err_stack)), &err_stack);
  }

  ZX_ERR_TRY_FATAL((FioUring_init(
                       &(FioUringParams){
                           .wq_fd = 0,
                           .polled_io = true,
                           .io_buf = nullptr,
                           .io_buf_size = 0,
                       },
                       &poll_ring, &err_stack)),
                   &err_stack);
  ZX_ERR_TRY_FATAL((FioUring_init(
                       &(FioUringParams){
                           .wq_fd = (zx_u32)poll_ring.ring_fd,
                           .polled_io = false,
                           .io_buf = nullptr,
                           .io_buf_size = 0,
                       },
                       &ring, &err_stack)),
                   &err_stack);

  constexpr zx_u32 total = 10;
  constexpr zx_u32 concurrency = 3;
  char const *msg = "Helllo!\n";
  zx_u32 to_submit = total;
  zx_u32 n_pending = 0;
  zx_u32 n_finished = 0;
  while (n_finished < total) {
    while (n_pending < concurrency && to_submit > 0) {
      ZX_ASSERT((FioUring_submit_write(&ring, ZX_STDOUT, (zx_u8 const *)msg,
                                       100, (zx_u32)__builtin_strlen(msg))));
      to_submit--;
      n_pending++;
    }

    FioCQE const *cqe = FioUring_pop_cqe(&ring);
    while (cqe != nullptr) {
      ZX_ASSERT((n_pending > 0));
      n_finished += 1;
      n_pending -= 1;

      cqe = FioUring_pop_cqe(&ring);
    }

    ZX_ERR_TRY_FATAL((FioUring_sync(&ring, &err_stack)), &err_stack);
  }

  ZX_ERR_TRY_FATAL((FioUring_deinit(&ring, &err_stack)), &err_stack);
  ZX_ERR_TRY_FATAL((FioUring_deinit(&poll_ring, &err_stack)), &err_stack);
  return 0;
}

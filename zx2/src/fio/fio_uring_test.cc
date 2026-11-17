#include <fio/fio_uring.h>
#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>
#include <zx/zx_sys_io_uring.h>

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  FioUring poll_ring;
  FioUring ring;

  FioUringParams poll_ring_params = FioUringParams{
      .wq_fd = 0,
      .polled_io = true,
      .io_buf = nullptr,
      .io_buf_size = 0,
  };

  FioUringParams ring_params = FioUringParams{
      .wq_fd = 0,
      .polled_io = false,
      .io_buf = nullptr,
      .io_buf_size = 0,
  };

  for (zx_u32 i = 0; i < 5; i++) {
    ZX_ERR_TRY_FATAL(
        (FioUring::init(&poll_ring_params, &poll_ring, &err_stack)),
        &err_stack);
    ring_params.wq_fd = poll_ring.wq_fd();
    ZX_ERR_TRY_FATAL((FioUring::init(&ring_params, &ring, &err_stack)),
                     &err_stack);
    ZX_ERR_TRY_FATAL((ring.deinit(&err_stack)), &err_stack);
    ZX_ERR_TRY_FATAL((poll_ring.deinit(&err_stack)), &err_stack);
  }

  ZX_ERR_TRY_FATAL((FioUring::init(&poll_ring_params, &poll_ring, &err_stack)),
                   &err_stack);
  ring_params.wq_fd = poll_ring.wq_fd();
  ZX_ERR_TRY_FATAL((FioUring::init(&ring_params, &ring, &err_stack)),
                   &err_stack);

  constexpr zx_u32 total = 10;
  constexpr zx_u32 concurrency = 3;
  char const *msg = "Helllo!\n";
  zx_u32 to_submit = total;
  zx_u32 n_pending = 0;
  zx_u32 n_finished = 0;
  while (n_finished < total) {
    while (n_pending < concurrency && to_submit > 0) {
      FioSQE *sqe = ring.push_sqe_in_place();
      ZX_ASSERT((sqe != nullptr));

      fio_uring_prep_write(sqe, ZX_STDOUT, (zx_u8 const *)msg, 100,
                           (zx_u32)__builtin_strlen(msg));

      to_submit--;
      n_pending++;
    }

    FioCQE const *cqe = ring.pop_cqe_in_place();
    while (cqe != nullptr) {
      ZX_ASSERT((n_pending > 0));
      n_finished += 1;
      n_pending -= 1;

      cqe = ring.pop_cqe_in_place();
    }

    ZX_ERR_TRY_FATAL((ring.sync(&err_stack)), &err_stack);
  }

  ZX_ERR_TRY_FATAL((ring.deinit(&err_stack)), &err_stack);
  ZX_ERR_TRY_FATAL((poll_ring.deinit(&err_stack)), &err_stack);
  return 0;
}

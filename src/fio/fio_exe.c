#include <fiber/fiber.h>
#include <fio/fio_exe.h>
#include <fio/fio_uring.h>
#include <fio/fio_wake.h>
#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_slab.h>

constexpr zx_u32 FIO_FIBER_STACK_SIZE = 1 << 18;
static_assert(FIO_FIBER_STACK_SIZE % FIBER_STACK_ALIGN == 0);

ZXStatus FioExe_init(ZXAlloc *alloc, FioExeParams const *params, FioExe *exe,
                     ZXErrStack *err_stack) {
  ZX_ASSERT((params->max_n_fibers < ZX_U32_MAX));
  ZX_ASSERT((params->max_n_futures < ZX_U32_MAX));

  bool did_init_io_ring = false;
  bool did_init_polled_io_ring = false;
  ZXStatus error_status = ZXStatus_Ok;

  FioUringParams uring_params = {
      .wq_fd = params->wq_fd,
      .polled_io = false,
      .io_buf = params->io_buf,
      .io_buf_size = params->io_buf_size,
  };
  FioUring io_ring;
  error_status = FioUring_init(&uring_params, &io_ring, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  did_init_io_ring = true;

  FioUring polled_io_ring;
  uring_params.polled_io = true;
  uring_params.wq_fd = FioUring_wq_fd(&io_ring);
  error_status = FioUring_init(&uring_params, &polled_io_ring, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }
  did_init_polled_io_ring = true;

  zx_u8 *fiber_stacks = nullptr;
  zx_u64 fiber_stacks_size =
      zx_chk_mul_u64(params->max_n_fibers,
                     zx_chk_add_u32(FIO_FIBER_STACK_SIZE, FIBER_RED_ZONE_SIZE));
  error_status = ZXAlloc_alloc(alloc, FIBER_STACK_ALIGN, fiber_stacks_size,
                               &fiber_stacks, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  ZXSlab fibers;
  error_status = ZXSlab_init(alloc, params->max_n_fibers, alignof(FioFiber),
                             sizeof(FioFiber), &fibers, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  ZXSlab futures;
  error_status = ZXSlab_init(alloc, params->max_n_futures, alignof(FioFuture),
                             sizeof(FioFuture), &futures, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  FioWake fiber_wake;
  error_status =
      FioWake_init(alloc, params->max_n_fibers, &fiber_wake, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  FioWake future_wake;
  error_status =
      FioWake_init(alloc, params->max_n_futures, &future_wake, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  zx_i32 *io_results = nullptr;
  zx_u32 const n_io_results =
      zx_chk_mul_u32(params->max_n_futures, FIO_IO_PER_FUTURE);
  zx_u64 const io_results_byte_size =
      zx_chk_mul_u64(n_io_results, sizeof(zx_i32));
  error_status = ZXAlloc_alloc(alloc, alignof(zx_i32), io_results_byte_size,
                               (zx_u8 **)&io_results, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  *exe = (FioExe){
      .io_ring = io_ring,
      .polled_io_ring = polled_io_ring,
      .fiber_wake = fiber_wake,
      .fibers = fibers,
      .fiber_stacks = fiber_stacks,
      .future_wake = future_wake,
      .futures = futures,
      .io_results = io_results,
      .yield_after_n_ticks = params->yield_after_n_ticks,
  };

  return ZXStatus_Ok;
err:
  if (did_init_polled_io_ring) {
    ZX_ERR_TRY_FATAL((FioUring_deinit(&polled_io_ring, err_stack)), err_stack);
  }
  if (did_init_io_ring) {
    ZX_ERR_TRY_FATAL((FioUring_deinit(&io_ring, err_stack)), err_stack);
  }

  ZX_ASSERT((error_status != ZXStatus_Ok));
  ZX_ERR_TRY(error_status, err_stack);

  ZX_ASSERT((false));
  return ZXStatus_Ok;
}

ZXStatus FioExe_deinit(FioExe *exe, ZXErrStack *err_stack) {
  ZX_ERR_TRY((FioUring_deinit(&exe->polled_io_ring, err_stack)), err_stack);
  ZX_ERR_TRY((FioUring_deinit(&exe->io_ring, err_stack)), err_stack);

  __builtin_memset(exe, 0, sizeof(FioExe));

  return ZXStatus_Ok;
}

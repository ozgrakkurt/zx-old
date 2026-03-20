#include "./fio_uring.h"
#include <fiber/fiber.h>
#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_queue.h>
#include <zx/zx_slab.h>

typedef struct FioCtx {
} FioCtx;

typedef bool fio_fut_pollfn(FioCtx const *ctx, void *state);

typedef struct FioFut {
  void *state;
  fio_fut_pollfn pollfn;
} FioFut;

typedef struct FioFiber {
  FiberCtx fib_ctx;
} FioFiber;

typedef struct FioWakeQueue {
  FioBitVec dedup;
  ZXQueue to_wake;
} FioWakeQueue;

typedef struct FioExe {
  // io_uring interface for regular IO
  FioUring io_ring;
  // io_uring interface for Direct IO
  FioUring polled_io_ring;
  // book-keeping for fibers
  FioWakeQueue fiber_wake;
  ZXSlab fiber;
  zx_u8 *fiber_stack;
  // book-keeping for Future objects
  FioWakeQueue fut_wake;
  ZXSlab fut;
  // Handle for sharing kernel SQ_POLL between different instances
  zx_u32 wq_fd;
} FioExe;

typedef struct FioExeParams {
  // Handle for sharing kernel SQ_POLL between different instances
  // 0 means no handle.
  zx_u32 wq_fd;
  // Maximum number of fibers that can be running at a time.
  // Attempting to spawn more fibers will error.
  zx_u32 max_n_fibers;
  // IO buffer for registering with io_uring. All later buffers passed to do io
  // operations have to be inside the range of this buffer. passing nullptr for
  // io_buf and zero for io_buf_size means "don't register anything". In that
  // case, it is not possible to do io operations that require passing a buffer
  // including file read/write and socket read/write.
  zx_u8 *io_buf;
  zx_u64 io_buf_size;
} FioExeParams;

constexpr zx_u32 FIO_FIBER_STACK_SIZE = 1 << 18;
static_assert(FIO_FIBER_STACK_SIZE % FIBER_STACK_ALIGN == 0);

static ZXStatus FioExe_init(ZXAlloc *alloc, FioExeParams const *params,
                            FioExe *exe, ZXErrStack *err_stack) {
  ZX_ASSERT((params->max_n_fibers > 0));

  bool did_init_io_ring = false;
  bool did_init_polled_io_ring = false;
  ZXStatus error_status = ZXStatus_Ok;

  FioUringParams uring_params = {
      .wq_fd = params->wq_fd,
      .polled_io = false,
      .io_buf = params->io_buf,
      .io_buf_size = params->io_size,
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

  zx_u8 *fiber_stack = nullptr;
  zx_u64 fiber_stack_size =
      zx_chk_mul_u64(params->max_n_fibers, FIO_FIBER_STACK_SIZE);
  error_status = ZXAlloc_alloc(alloc, FIBER_STACK_ALIGN, fiber_stack_size,
                               &stacks, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  ZXSlab fiber;
  error_status = ZXSlab_init(alloc, params->max_n_fibers, alignof(FioFiber),
                             sizeof(FioFiber), &fiber, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  ZXSlab fut;
  error_status = ZXSlab_init(alloc, params->max_n_fibers, alignof(FioFut),
                             sizeof(FioFut), &fut, err_stack);
  if (error_status != ZXStatus_Ok) {
    ZX_ERR_APPEND_LOC(err_stack);
    goto err;
  }

  *exe = (FioExe){
      .io_ring = io_ring,
      .polled_io_ring = polled_io_ring,
      .fiber_wake = fiber_wake,
      .fiber = fiber,
      .fiber_stack = fiber_stack,
      .fut_wake = fut_wake,
      .fut = fut,
      .wq_fd = FioUring_wq_fd(&io_ring),
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

static ZXStatus FioExe_deinit(FioExe *exe, ZXErrStack *err_stack) {
  ZX_ERR_TRY((FioUring_deinit(&exe->polled_io_ring, err_stack)), err_stack);
  ZX_ERR_TRY((FioUring_deinit(&exe->io_ring, err_stack)), err_stack);

  return ZXStatus_Ok;
}

static zx_u32 FioExe_wq_fd(FioExe const *exe) { return exe->wq_fd; }

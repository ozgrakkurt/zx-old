#include "./fio_uring.h"
#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_queue.h>
#include <zx/zx_slab.h>

typedef struct FioExe {
  // io_uring interface for regular IO
  FioUring io_ring;
  // io_uring interface for Direct IO
  FioUring polled_io_ring;
  // book-keeping for fibers
  bool *fiber_wake;
  ZXSlab fibers;
  // book-keeping for io objects
  bool *io_wake;
  ZXSlab ios;
  // indivdual requests into io_uring
  ZXSlab ops;
} FioExe;

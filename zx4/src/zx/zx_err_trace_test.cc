#include <zx/zx_alloc.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

static void assert(bool v) {
  if (!v) {
    zx_panic();
  }
}

static void do_test_nocap() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());
  ZXErrTrace trace;
  ZXErrTraceInitStatus status = ZXErrTrace::init(&noalloc, 1, &trace);
  assert(status == ZXErrTraceInitStatus::OutOfMemory);

  status = ZXErrTrace::init(&noalloc, 0, &trace);
  assert(status == ZXErrTraceInitStatus::Ok);

  bool success = zx_err_trace_push(&trace, ZX_SRC_LOC, 11);
  assert(!success);

  trace.reset();

  success = zx_err_trace_push(&trace, ZX_SRC_LOC, 11);
  assert(!success);

  trace.reset();

  success = zx_err_trace_push(&trace, ZX_SRC_LOC, 11);
  assert(!success);
}

static void do_test(ZXAlloc *alloc) {
  ZXSliceMut<zx_u8> fmt_buf;
  ZXAllocStatus alloc_status = alloc->alloc<zx_u8>(1 << 12, &fmt_buf);
  assert(alloc_status == ZXAllocStatus::Ok);
  ZXFmt fmt = ZXFmt::init(fmt_buf);

  ZXErrTrace trace;
  ZXErrTraceInitStatus status = ZXErrTrace::init(alloc, 3, &trace);
  assert(status == ZXErrTraceInitStatus::Ok);

  bool success = zx_err_trace_push(&trace, ZX_SRC_LOC, 0);
  assert(success);
  success = zx_err_trace_push(&trace, ZX_SRC_LOC, 1);
  assert(success);
  success = zx_err_trace_push(&trace, ZX_SRC_LOC, 2);
  assert(success);
  success = zx_err_trace_push(&trace, ZX_SRC_LOC, 255);
  assert(!success);

  fmt.reset();
  bool overflow = false;
  trace.format(&fmt, &overflow);
  assert(!overflow);

  // {
  //   zx_u64 n_written = 0;
  //   ZXSysStatus const w_status =
  //       zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
  //   (void)w_status;
  //   (void)n_written;
  // }

  ZXSlice<zx_u8> const expected =
      ZXCString::init_unsafe("zx_err_trace_test.cc[line=49]. StatusCode = 0\n"
                             "zx_err_trace_test.cc[line=51]. StatusCode = 1\n"
                             "zx_err_trace_test.cc[line=53]. StatusCode = 2\n")
          .slice();

  // {
  //   zx_u64 n_written = 0;
  //   ZXSysStatus const w_status = zx_sys_write(ZX_STDERR, expected.slice(),
  //   &n_written); (void)w_status; (void)n_written;
  // }

  assert(fmt.written().equals(expected));
}
// if (status != ZXErrTraceStatus::Ok) {
//   bool fmt_overflow = false;
//   fmt.push_cstring(zx_err_trace_status_to_cstring(status), &fmt_overflow);
//   fmt.push_char('\n', &fmt_overflow);
//   (void)fmt_overflow;
// zx_u64 n_written = 0;
// ZXSysStatus const w_status =
//     zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
// (void)w_status;
// (void)n_written;
//   zx_panic();
// }

int main(void) {
  ZXSliceMut<zx_u8> mem;
  ZXPageAllocStatus page_status = zx_page_alloc_4kb(1 << 17, &mem);
  assert(page_status == ZXPageAllocStatus::Ok);

  do_test_nocap();

  ZXAlloc alloc = ZXAlloc::init(mem);
  do_test(&alloc);

  page_status = zx_page_free(mem);
  assert(page_status == ZXPageAllocStatus::Ok);
  return 0;
}

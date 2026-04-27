#include <zx/zx_alloc.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

static void assert(bool v) {
  if (!v) {
    __builtin_trap();
  }
}

static void do_test_nocap() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());
  ZXErrTrace trace;
  ZXErrTraceStatus status = ZXErrTrace::init(&noalloc, 1, &trace);
  assert(status == ZXErrTraceStatus::OutOfMemory);

  status = ZXErrTrace::init(&noalloc, 0, &trace);
  assert(status == ZXErrTraceStatus::Ok);

  bool success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("xd"));
  assert(!success);

  trace.reset();

  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("xd"));
  assert(!success);

  trace.reset();

  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("xd"));
  assert(!success);
}

static void do_test(ZXAlloc* alloc) {
  ZXSliceMut<zx_u8> fmt_buf;
  ZXAllocStatus alloc_status = alloc->alloc<zx_u8>(1 << 12, &fmt_buf);
  assert(alloc_status == ZXAllocStatus::Ok);
  ZXFmt fmt = ZXFmt::init(fmt_buf);

  ZXErrTrace trace;
  ZXErrTraceStatus status = ZXErrTrace::init(alloc, 3, &trace);
  assert(status == ZXErrTraceStatus::Ok);

  bool success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("inf0"));
  assert(success);
  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("inf1"));
  assert(success);
  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("inf2"));
  assert(success);
  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("inf69"));
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
      ZXCString::init_unsafe(
          "zx_err_trace_test.cc[line=48]. Info = inf0\n"
          "zx_err_trace_test.cc[line=50]. Info = inf1\n"
          "zx_err_trace_test.cc[line=52]. Info = inf2\n")
          .slice();

  // {
  //   zx_u64 n_written = 0;
  //   ZXSysStatus const w_status = zx_sys_write(ZX_STDERR, expected.slice(),
  //   &n_written); (void)w_status; (void)n_written;
  // }

  assert(fmt.written().equals(expected));

  assert(trace.pop());

  fmt.reset();
  overflow = false;
  trace.format(&fmt, &overflow);
  assert(!overflow);

  ZXSlice<zx_u8> const expected2 =
      ZXCString::init_unsafe(
          "zx_err_trace_test.cc[line=48]. Info = inf0\n"
          "zx_err_trace_test.cc[line=50]. Info = inf1\n")
          .slice();

  assert(fmt.written().equals(expected2));

  success = trace.push(ZX_SRC_LOC, ZXCString::init_unsafe("inf699"));
  assert(success);

  fmt.reset();
  overflow = false;
  trace.format(&fmt, &overflow);
  assert(!overflow);

  ZXSlice<zx_u8> const expected3 =
      ZXCString::init_unsafe(
          "zx_err_trace_test.cc[line=48]. Info = inf0\n"
          "zx_err_trace_test.cc[line=50]. Info = inf1\n"
          "zx_err_trace_test.cc[line=100]. Info = inf699\n")
          .slice();

  assert(fmt.written().equals(expected3));

  assert(trace.pop());
  assert(trace.pop());
  assert(trace.pop());
  assert(!trace.pop());
  assert(!trace.pop());
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
//   __builtin_trap();
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

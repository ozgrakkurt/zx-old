#ifndef __linux__
#error "only linux is supported"
#endif

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#include <zx/zx_alloc.h>
#include <zx/zx_arr.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_print.h>
#include <zx/zx_slice.h>
#include <zx/zx_statx.h>
#include <zx/zx_sys.h>

enum class [[nodiscard]] ZXBacktraceStatus : zx_u8_c {
  Ok = 0,
  ArrayPushFail,
  ArrayOutOfCapacity,
  Nullptr,
};

static ZXBacktraceStatus zx_backtrace(ZXErrTrace *err_trace,
                                      ZXArr<zx_u64> *addrs) {
  if (addrs == nullptr) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, 0);
    return ZXBacktraceStatus::Nullptr;
  }

  zx_u64 *frame_ptr = (zx_u64 *)__builtin_frame_address(0);
  while (frame_ptr != nullptr) {
    zx_u64 const instr_ptr = zx_wrapping_sub_u64(frame_ptr[1], 1);

    ZXArrPushStatus const push_status = addrs->push(&instr_ptr);
    switch (push_status) {
    case ZXArrPushStatus::Ok: {
      break;
    }
    case ZXArrPushStatus::OutOfCapacity: {
      zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)push_status);
      return ZXBacktraceStatus::ArrayOutOfCapacity;
    }
    default: {
      zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)push_status);
      return ZXBacktraceStatus::ArrayPushFail;
    }
    }

    frame_ptr = (zx_u64 *)frame_ptr[0];
  }

  return ZXBacktraceStatus::Ok;
}

enum class [[nodiscard]] ZXPanicReadSelfStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OpenFileFailed,
  StatxFailed,
  ShortRead,
  LongRead,
  ReadFailed,
  CloseFailed,
};

static ZXPanicReadSelfStatus zx_panic_read_self(ZXErrTrace *err_trace,
                                                ZXSliceMut<zx_u8> file_buf,
                                                zx_u64 *n_bytes_read) {
  if (n_bytes_read == nullptr) {
    return ZXPanicReadSelfStatus::Nullptr;
  }

  ZXPanicReadSelfStatus out_status = ZXPanicReadSelfStatus::Ok;

  zx_i32 fd = -1;
  zx_u64 n_read = 0;
  statx stats;
  __builtin_memset(&stats, 0, sizeof(stats));

  if (ZXSysStatus const status = zx_sys_open(
          ZXCString::init_unsafe("/proc/self/exe"), ZX_SYS_O_RDONLY, 0, &fd);
      ZXSysStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    out_status = ZXPanicReadSelfStatus::OpenFileFailed;
    goto cleanup;
  }

  if (ZXSysStatus const status =
          zx_sys_statx(fd, ZXCString::init_unsafe(""), ZX_SYS_AT_EMPTY_PATH,
                       STATX_SIZE, &stats);
      ZXSysStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    out_status = ZXPanicReadSelfStatus::StatxFailed;
    goto cleanup;
  }

  if (ZXSysStatus const status = zx_sys_read(fd, file_buf, &n_read);
      ZXSysStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    out_status = ZXPanicReadSelfStatus::ReadFailed;
    goto cleanup;
  }

  if (n_read < stats.stx_size) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC,
                      (zx_u8)ZXPanicReadSelfStatus::ShortRead);
    out_status = ZXPanicReadSelfStatus::ShortRead;
    goto cleanup;
  } else if (n_read > stats.stx_size) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC,
                      (zx_u8)ZXPanicReadSelfStatus::LongRead);
    out_status = ZXPanicReadSelfStatus::LongRead;
    goto cleanup;
  }

  *n_bytes_read = n_read;
cleanup:
  if (ZXSysStatus const status = zx_sys_close(fd); ZXSysStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    return ZXPanicReadSelfStatus::CloseFailed;
  }

  return out_status;
}

enum class [[nodiscard]] ZXPanicPrintInfoStatus : zx_u8_c {
  Ok = 0,
  OutOfMemory,
  AllocFail,
  BacktraceFail,
  Nullptr,
  AddrsInitFail,
  ReadSelfFail,
};

static ZXPanicPrintInfoStatus zx_panic_print_info(ZXErrTrace *err_trace,
                                                  ZXAlloc *alloc, ZXFmt *fmt) {
  if (alloc == nullptr || fmt == nullptr) {
    return ZXPanicPrintInfoStatus::Nullptr;
  }

  ZXSliceMut<zx_u8> file_buf;
  switch (ZXAllocStatus const status = alloc->alloc<zx_u8>(1 << 26, &file_buf);
          status) {
  case ZXAllocStatus::Ok: {
    break;
  }
  case ZXAllocStatus::OutOfMemory: {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    return ZXPanicPrintInfoStatus::OutOfMemory;
    break;
  }
  default: {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    return ZXPanicPrintInfoStatus::AllocFail;
    break;
  }
  }

  ZXArr<zx_u64> addrs;
  if (ZXArrInitStatus const status = ZXArr<zx_u64>::init(alloc, 64, &addrs);
      ZXArrInitStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, (zx_u8)status);
    return ZXPanicPrintInfoStatus::AddrsInitFail;
  }

  switch (ZXBacktraceStatus const status = zx_backtrace(err_trace, &addrs);
          status) {
  case ZXBacktraceStatus::Ok: {
    break;
  }
  case ZXBacktraceStatus::ArrayOutOfCapacity: {
    ZX_EPRINT("Backtrace was truncated because it went out of capacity.\n "
              "Resulting stack trace will be missing some frames.\n");
    break;
  }
  default: {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, 0);
    return ZXPanicPrintInfoStatus::BacktraceFail;
    break;
  }
  }

  fmt->reset();
  ZXSlice<zx_u64> const addrs_slice = addrs.slice();
  bool overflow = false;
  for (zx_u64 i = 0; i < addrs_slice.len(); i++) {
    zx_u64 const addr = addrs_slice.read_unsafe(i);
    fmt->push_u64_hex(addr, &overflow);
    fmt->push_char('\n', &overflow);
  }

  if (overflow) {
    ZX_EPRINT("String formatting overflowed while printing panic info. \n "
              "Resulting output will be missing some content.\n");
  }

  zx_u64 n_written = 0;
  (void)zx_sys_write(ZX_STDERR, fmt->written(), &n_written);
  (void)n_written;

  zx_u64 self_exe_size = 0;
  if (ZXPanicReadSelfStatus const status =
          zx_panic_read_self(err_trace, file_buf, &self_exe_size);
      ZXPanicReadSelfStatus::Ok != status) {
    zx_err_trace_push(err_trace, ZX_SRC_LOC, 0);
    return ZXPanicPrintInfoStatus::ReadSelfFail;
  }

  fmt->reset();
  overflow = false;
  ZX_FMT(fmt, &overflow, "Length of self exe: ", self_exe_size, '\n');

  n_written = 0;
  (void)zx_sys_write(ZX_STDERR, fmt->written(), &n_written);
  (void)n_written;

  return ZXPanicPrintInfoStatus::Ok;
}

static bool ZX_PANICKING_ = false;

[[noreturn]] void zx_panic(void) {
  // Avoid infinite looping.
  // TODO: Make this compatible with panicking from multiple threads while still
  // avoiding infinite looping.
  if (__atomic_exchange_n(&ZX_PANICKING_, true, __ATOMIC_RELAXED)) {
    ZX_EPRINT("The program was already panicking while trying to panic.\n This "
              "could be because of the panic function panicking itself or "
              "multiple threads panicking simultaneously.\n Use a debugger to "
              "find out what is happening.\n");
    __builtin_trap();
  }
  ZXSliceMut<zx_u8> mem;
  if (ZXPageAllocStatus const status = zx_page_alloc_4kb(1 << 27, &mem);
      ZXPageAllocStatus::Ok != status) {
    ZX_EPRINT("Failed to allocate memory for panic debugging. Error code: ",
              (zx_u8)status, '\n');
    __builtin_trap();
  }
  ZXAlloc alloc = ZXAlloc::init(mem);

  ZXErrTrace err_trace;
  if (ZXErrTraceInitStatus const status =
          ZXErrTrace::init(&alloc, 32, &err_trace);
      ZXErrTraceInitStatus::Ok != status) {
    ZX_EPRINT("Failed to create ZXErrTrace for panic debugging. Error code: ",
              (zx_u8)status, '\n');
  }

  ZXSliceMut<zx_u8> fmt_mem;
  if (ZXAllocStatus::Ok != alloc.alloc<zx_u8>(1 << 14, &fmt_mem)) {
    ZX_EPRINT("Failed to alloc memory for string formatting.\n");
    __builtin_trap();
  }
  ZXFmt fmt = ZXFmt::init(fmt_mem);

  if (ZXPanicPrintInfoStatus const status =
          zx_panic_print_info(&err_trace, &alloc, &fmt);
      ZXPanicPrintInfoStatus::Ok != status) {
    ZX_EPRINT("Failed to print panic info. Error code: ", (zx_u8)status,
              "\n. Printing error trace for panic info printing func: \n");

    fmt.reset();

    bool overflow = false;
    err_trace.format(&fmt, &overflow);

    if (overflow) {
      ZX_EPRINT("Panic info Err trace formatting overflowed. Resulting output "
                "might be cropped.\n");
    }

    zx_u64 n_written = 0;
    (void)zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
    (void)n_written;

    __builtin_trap();
  }

  __builtin_trap();
}

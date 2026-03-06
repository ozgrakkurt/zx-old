#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_sys.h>

#define ZX_ERR_TRACE_TRY_ABORT(expr, stack_var)                                \
  do {                                                                         \
    char const *zx_err_trace_abort_info_ =                                     \
        "Aborting because of error at:\n" ZX_ERR_INFO_(                        \
            ZX_ERR_TRACE_TRY_ABORT(expr));                                     \
    ZXStatus const zx_err_trace_abort_status_ = (expr);                        \
    if (zx_err_trace_abort_status_ != ZXStatus_Ok) {                           \
      ZX_FUZZ_ABORT_MARKER;                                                    \
      zx_err_trace_abort_(zx_err_trace_abort_info_,                            \
                          __builtin_strlen(zx_err_trace_abort_info_),          \
                          (stack_var));                                        \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

// Outputs the trace to stderr and aborts the program
[[noreturn]] void zx_err_trace_abort(ZXErrStack const *stack);

// Outputs a string to trace and the length of the string to trace_len
ZXStatus zx_err_trace_collect(ZXAlloc *alloc, ZXErrStack const *stack,
                              char const **trace, zx_usize *trace_len,
                              ZXErrStack *err_stack);

[[noreturn]] static inline void zx_err_trace_abort_(char const *info,
                                                    zx_usize info_len,
                                                    ZXErrStack const *stack) {
  zx_usize n_written = 0;
  zx_usize const w_status = zx_sys_write(ZX_STDERR, info, info_len, &n_written);
  (void)w_status;
  zx_err_trace_abort(stack);
}

#endif

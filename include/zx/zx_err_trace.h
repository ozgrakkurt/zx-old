#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>

#define ZX_ERR_COLOR_RED_ "\x1B[31m"
#define ZX_ERR_COLOR_RESET_ "\x1B[0m"

#define ZX_ERR_TRACE_ABORT(expr, stack_var)                                    \
  do {                                                                         \
    char const *zx_err_trace_abort_info_ =                                     \
        ZX_ERR_COLOR_RED_ "Aborting because of error at:\n" ZX_ERR_INFO_(      \
            ZX_ERR_TRACE_ABORT(expr)) ZX_ERR_COLOR_RESET_ "\n";                \
    zx_err_trace_abort_((expr), zx_err_trace_abort_info_,                      \
                        __builtin_strlen(zx_err_trace_abort_info_),            \
                        (stack_var));                                          \
  } while (false)

// Outputs the trace to stderr and aborts the program
[[noreturn]] void zx_err_trace_abort(ZXErrStack const *stack);

// Outputs a string to trace and the length of the string to trace_len
ZXStatus zx_err_trace_collect(ZXAlloc *alloc, ZXErrStack const *stack,
                              char const **trace, zx_usize *trace_len,
                              ZXErrStack *err_stack);

static inline void zx_err_trace_abort_(ZXStatus status, char const *info,
                                       zx_usize info_len,
                                       ZXErrStack const *stack) {
  if (status != ZXStatus_Ok) {
    zx_usize n_written = 0;
    zx_usize const w_status =
        zx_sys_write(ZX_STDERR, info, info_len, &n_written);
    (void)w_status;
    zx_err_trace_abort(stack);
  }
}

#endif

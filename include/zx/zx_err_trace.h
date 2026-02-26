#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_sys.h>

#define ZX_ERR_TRACE_ABORT(expr, stack_var)                                    \
  do {                                                                         \
    ZXStatus const status = (expr);                                            \
    ZXErrStack *const stack = (stack_var);                                     \
    if (status != ZXStatus_Ok) {                                               \
      char const *err_msg =                                                    \
          ZX_ERR_COLOR_RED "Aborting because of error at:\n" ZX_ERR_INFO(      \
              ZX_ERR_TRACE_ABORT(expr)) ZX_ERR_COLOR_RESET "\n";               \
      zx_usize n_written = 0;                                                  \
      (void)zx_sys_write(ZX_STDERR, err_msg, __builtin_strlen(err_msg),        \
                         &n_written);                                          \
      zx_err_trace_abort(stack);                                               \
    }                                                                          \
  } while (false)

// Outputs the trace to stderr and aborts the program
[[noreturn]] void zx_err_trace_abort(ZXErrStack const *stack);

// Outputs a string to trace and the length of the string to trace_len
ZXStatus zx_err_trace_collect(ZXAlloc *alloc, ZXErrStack const *stack,
                              char const **trace, zx_usize *trace_len,
                              ZXErrStack *err_stack);

#endif

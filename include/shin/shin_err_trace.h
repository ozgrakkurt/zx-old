#ifndef SHIN_ERR_TRACE_H
#define SHIN_ERR_TRACE_H

#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ERR_TRACE_ABORT(expr, stack_var)                           \
  do {                                                                  \
    ShinStatus const status = (expr);                                   \
    ShinErrStack* const stack = (stack_var);                            \
    if (status != ShinStatus_Ok) {                                      \
      fprintf(stderr,                                                   \
              SHIN_ERR_COLOR_RED                                        \
              "Aborting because of error at:\n%s" SHIN_ERR_COLOR_RESET, \
              SHIN_ERR_INFO(SHIN_ERR_TRACE_ABORT(expr)));               \
      shin_err_trace_abort(stack);                                      \
    }                                                                   \
  } while (false)

// Outputs the trace to stderr and aborts the program
[[noreturn]] void shin_err_trace_abort(ShinErrStack const* stack);

// Outputs a string to trace and the length of the string to trace_len
ShinStatus shin_err_trace_collect(ShinAlloc* alloc, ShinErrStack const* stack,
                                  char const** trace, shin_usize* trace_len,
                                  ShinErrStack* err_stack);

ShinStatus shin_err_trace_init(ShinAlloc* alloc, ShinErrStack* stack,
                               shin_usize depth);

#ifdef __cplusplus
}
#endif

#endif

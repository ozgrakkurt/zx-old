#ifndef SHIN_ALLOC_H
#define SHIN_ALLOC_H

#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ERR_TRACE_UNWRAP(expr, stack_var)       \
  do {                                               \
    ShinStatus const status = (expr);                \
    ShinErrStack* stack = (stack_var);               \
    if (status != ShinStatus_Ok) {                   \
      shin_u32 const depth = stack->depth;           \
      constexpr char info[] = SHIN_ERR_INFO(expr);   \
      stack->info_lengths[depth] = sizeof(info) - 1; \
      stack->infos[depth] = info;                    \
      if (depth + 1 < SHIN_ERR_STACK_CAP) {          \
        stack->depth = depth + 1;                    \
      }                                              \
      shin_stack_trace_panic(stack);                 \
    }                                                \
  } while (false)

void shin_err_trace_panic(ShinStatus status);

// Outputs a string to trace and the length of the string to trace_len
ShinStatus shin_err_trace_collect(ShinAlloc* alloc, ShinErrStack const* stack,
                                  char const** trace, size_t* trace_len);

#ifdef __cplusplus
}
#endif

#endif

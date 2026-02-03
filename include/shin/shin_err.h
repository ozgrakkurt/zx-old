#ifndef SHIN_ERR_H
#define SHIN_ERR_H

#include <shin/shin_def.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ERR_STRINGIFY(x) #x
#define SHIN_ERR_TO_STRING(x) SHIN_ERR_STRINGIFY(x)
#define SHIN_ERR_INFO(expr)                      \
  "FILE=" __FILE__ ": LINE=" SHIN_ERR_TO_STRING( \
      __LINE__) ": EXPR=(" SHIN_ERR_TO_STRING(expr) ")"

#ifdef SHIN_DBG_ASSERT_ENABLED
#define SHIN_DBG_ASSERT(expr)                                       \
  do {                                                              \
    bool const v = (expr);                                          \
    if (!v) {                                                       \
      fprintf(stderr, "Failed assertion at: " SHIN_ERR_INFO(expr)); \
      abort();                                                      \
    }                                                               \
  } while (false)
#else
#define SHIN_DBG_ASSERT(expr) ((void)0)
#endif

#define SHIN_ERR_TRY(expr, stack_var)                \
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
      return status;                                 \
    }                                                \
  } while (false)

#define SHIN_ERR_RETURN(expr, stack_var)         \
  do {                                           \
    ShinStatus const status = (expr);            \
    ShinErrStack* stack = (stack_var);           \
    constexpr char info[] = SHIN_ERR_INFO(expr); \
    stack->info_lengths[0] = sizeof(info) - 1;   \
    stack->infos[0] = info;                      \
    stack->depth = 1;                            \
    return status;                               \
  } while (false)

#define SHIN_ERR_ASSERT(expr, stack_var)           \
  do {                                             \
    bool const v = (expr);                         \
    if (!v) {                                      \
      ShinErrStack* stack = (stack_var);           \
      constexpr char info[] = SHIN_ERR_INFO(expr); \
      stack->info_lengths[0] = sizeof(info) - 1;   \
      stack->infos[0] = info;                      \
      stack->depth = 1;                            \
      return ShinStatus_AssertFail;                \
    }                                              \
  } while (false)

shin_u32 constexpr SHIN_ERR_STACK_CAP = 1 << 10;

typedef enum [[nodiscard]] ShinStatus : char {
  ShinStatus_Ok = 0,
  ShinStatus_UnexpectedError = 1,
  ShinStatus_AssertFail = 2,
  ShinStatus_OutOfMemory = 3,
} ShinStatus;

typedef struct ShinErrStack {
  char const** infos;
  shin_usize* info_lengths;
  shin_u32 depth;
} ShinErrStack;

#ifdef __cplusplus
}
#endif

#endif

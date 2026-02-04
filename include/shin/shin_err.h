#ifndef SHIN_ERR_H
#define SHIN_ERR_H

#include <shin/shin_def.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ERR_COLOR_RED "\x1B[31m"
#define SHIN_ERR_COLOR_RESET "\x1B[0m"

#define SHIN_ERR_STRINGIFY(x) #x
#define SHIN_ERR_TO_STRING(x) SHIN_ERR_STRINGIFY(x)
#define SHIN_ERR_INFO(expr)                        \
  "\tFILE=" __FILE__ "; LINE=" SHIN_ERR_TO_STRING( \
      __LINE__) "; EXPR=(" SHIN_ERR_TO_STRING(expr) ");\n"

#define SHIN_ASSERT(expr)                                      \
  do {                                                         \
    bool const v = (expr);                                     \
    if (!v) {                                                  \
      fprintf(stderr,                                          \
              SHIN_ERR_COLOR_RED                               \
              "Failed assertion at:\n%s" SHIN_ERR_COLOR_RESET, \
              SHIN_ERR_INFO(expr));                            \
      abort();                                                 \
    }                                                          \
  } while (false)

#ifdef SHIN_DBG_ASSERT_ENABLED
#define SHIN_DBG_ASSERT(expr) SHIN_ASSERT(expr)
#else
#define SHIN_DBG_ASSERT(expr) ((void)0)
#endif

#define SHIN_ERR_TRY(expr, stack_var)                       \
  do {                                                      \
    ShinStatus const status = (expr);                       \
    ShinErrStack* stack = (stack_var);                      \
    if (status != ShinStatus_Ok) {                          \
      shin_u32 const depth = stack->depth;                  \
      char const* info = SHIN_ERR_INFO(SHIN_ERR_TRY(expr)); \
      stack->info_lengths[depth] = strlen(info);            \
      stack->infos[depth] = info;                           \
      if (depth + 1 < SHIN_ERR_STACK_CAP) {                 \
        stack->depth = depth + 1;                           \
      }                                                     \
      return status;                                        \
    }                                                       \
  } while (false)

#define SHIN_ERR_RETURN(expr, stack_var)                     \
  do {                                                       \
    ShinStatus const status = (expr);                        \
    ShinErrStack* stack = (stack_var);                       \
    char const* info = SHIN_ERR_INFO(SHIN_ERR_RETURN(expr)); \
    stack->info_lengths[0] = strlen(info);                   \
    stack->infos[0] = info;                                  \
    stack->depth = 1;                                        \
    return status;                                           \
  } while (false)

#define SHIN_ERR_ASSERT(expr, stack_var)                       \
  do {                                                         \
    bool const v = (expr);                                     \
    if (!v) {                                                  \
      ShinErrStack* stack = (stack_var);                       \
      char const* info = SHIN_ERR_INFO(SHIN_ERR_ASSERT(expr)); \
      stack->info_lengths[0] = strlen(info);                   \
      stack->infos[0] = info;                                  \
      stack->depth = 1;                                        \
      return ShinStatus_AssertFail;                            \
    }                                                          \
  } while (false)

typedef enum [[nodiscard]] ShinStatus : char {
  ShinStatus_Ok = 0,
  ShinStatus_UnexpectedError = 1,
  ShinStatus_AssertFail = 2,
  ShinStatus_OutOfMemory = 3,
} ShinStatus;

constexpr shin_u32 SHIN_ERR_STACK_CAP = 1 << 9;

typedef struct ShinErrStack {
  char const* infos[SHIN_ERR_STACK_CAP];
  shin_usize info_lengths[SHIN_ERR_STACK_CAP];
  shin_u32 depth;
} ShinErrStack;

static inline ShinErrStack ShinErrStack_init() {
  return (ShinErrStack){
      .infos = {0},
      .info_lengths = {0},
      .depth = 0wbu,
  };
}

#ifdef __cplusplus
}
#endif

#endif

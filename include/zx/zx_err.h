#ifndef ZX_ERR_H
#define ZX_ERR_H

#include <zx/zx_def.h>

#define ZX_ERR_COLOR_RED_ "\x1B[31m"
#define ZX_ERR_COLOR_RESET_ "\x1B[0m"

#define ZX_ERR_STRINGIFY_(x) #x
#define ZX_ERR_TO_STRING_(x) ZX_ERR_STRINGIFY_(x)
#define ZX_ERR_INFO_(expr)                                                     \
  "\tFILE=" __FILE__ "; LINE=" ZX_ERR_TO_STRING_(                              \
      __LINE__) "; EXPR=(" ZX_ERR_TO_STRING_(expr) ");\n"

#define ZX_ERR_TRY(expr, stack_var)                                            \
  do {                                                                         \
    ZXStatus const status = (expr);                                            \
    ZXErrStack *stack = (stack_var);                                           \
    if (status != ZXStatus_Ok) {                                               \
      zx_u32 const depth = stack->depth;                                       \
      zx_u32 const write_idx =                                                 \
          (depth == ZX_ERR_STACK_CAP_) ? (ZX_ERR_STACK_CAP_ - 1) : depth;      \
      stack->info_lengths[write_idx] =                                         \
          sizeof(ZX_ERR_INFO_(ZX_ERR_TRY(expr))) - 1;                          \
      stack->infos[write_idx] = ZX_ERR_INFO_(ZX_ERR_TRY(expr));                \
      stack->depth = write_idx + 1;                                            \
      return status;                                                           \
    }                                                                          \
  } while (false)

#define ZX_ERR_RETURN(expr, stack_var)                                         \
  do {                                                                         \
    ZXStatus const status = (expr);                                            \
    ZXErrStack *stack = (stack_var);                                           \
    stack->info_lengths[0] = sizeof(ZX_ERR_INFO_(ZX_ERR_RETURN(expr))) - 1;    \
    stack->infos[0] = ZX_ERR_INFO_(ZX_ERR_RETURN(expr));                       \
    stack->depth = 1;                                                          \
    return status;                                                             \
  } while (false)

#define ZX_ERR_ASSERT(expr, stack_var)                                         \
  do {                                                                         \
    bool const v = (expr);                                                     \
    if (!v) {                                                                  \
      ZXErrStack *stack = (stack_var);                                         \
      stack->info_lengths[0] = sizeof(ZX_ERR_INFO_(ZX_ERR_ASSERT(expr))) - 1;  \
      stack->infos[0] = ZX_ERR_INFO_(ZX_ERR_ASSERT(expr));                     \
      stack->depth = 1;                                                        \
      return ZXStatus_AssertFail;                                              \
    }                                                                          \
  } while (false)

typedef enum [[nodiscard]] ZXStatus : unsigned char {
  ZXStatus_Ok = 0,
  ZXStatus_UnexpectedError,
  ZXStatus_AssertFail,
  ZXStatus_OutOfMemory,
} ZXStatus;

constexpr zx_u32 ZX_ERR_STACK_CAP_ = 64;

typedef struct ZXErrStack {
  char const *infos[ZX_ERR_STACK_CAP_];
  zx_usize info_lengths[ZX_ERR_STACK_CAP_];
  zx_u32 depth;
} ZXErrStack;

static inline ZXErrStack ZXErrStack_init() {
  return (ZXErrStack){
      .infos = {0},
      .info_lengths = {0},
      .depth = 0wbu,
  };
}

#endif

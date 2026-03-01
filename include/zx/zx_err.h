#ifndef ZX_ERR_H
#define ZX_ERR_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_sys.h>

#define ZX_ERR_STRINGIFY_(x) #x
#define ZX_ERR_TO_STRING_(x) ZX_ERR_STRINGIFY_(x)
#define ZX_ERR_INFO_(expr)                                                     \
  "\tFILE=" __FILE__ "; LINE=" ZX_ERR_TO_STRING_(                              \
      __LINE__) "; EXPR=(" ZX_ERR_TO_STRING_(expr) ");\n"

#define ZX_ERR_TRY(expr, stack_var)                                            \
  do {                                                                         \
    ZXStatus const zx_err_try_status_ = (expr);                                \
    char const *zx_err_try_info_ = ZX_ERR_INFO_(ZX_ERR_TRY(expr));             \
    if (zx_err_try_(zx_err_try_status_, zx_err_try_info_,                      \
                    __builtin_strlen(zx_err_try_info_), (stack_var))) {        \
      return zx_err_try_status_;                                               \
      ZX_FUZZ_ERR;                                                             \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

#define ZX_ERR_RETURN(expr, stack_var)                                         \
  do {                                                                         \
    ZXStatus const zx_err_return_status_ = (expr);                             \
    ZX_ASSERT((zx_err_return_status_ != ZXStatus_Ok));                         \
    char const *zx_err_return_info_ = ZX_ERR_INFO_(ZX_ERR_RETURN(expr));       \
    zx_err_return_(zx_err_return_info_, __builtin_strlen(zx_err_return_info_), \
                   (stack_var));                                               \
    ZX_FUZZ_ERR;                                                               \
    return zx_err_return_status_;                                              \
  } while (false)

#define ZX_ERR_ASSERT(expr, stack_var)                                         \
  do {                                                                         \
    char const *zx_err_assert_info_ = ZX_ERR_INFO_(ZX_ASSERT_RETURN(expr));    \
    if (!zx_err_assert_((expr), zx_err_assert_info_,                           \
                        __builtin_strlen(zx_err_assert_info_), (stack_var))) { \
      ZX_FUZZ_ERR;                                                             \
      return ZXStatus_AssertFail;                                              \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

typedef enum [[nodiscard]] ZXStatus : unsigned char {
  ZXStatus_Ok = 0,
  ZXStatus_UnexpectedError,
  ZXStatus_AssertFail,
  ZXStatus_OutOfMemory,
  ZXStatus_ShortWrite,
  ZXStatus_LongWrite,
  ZXStatus_SysErrUnexpected,
} ZXStatus;

constexpr zx_u32 ZX_ERR_STACK_CAP_ = 64;

typedef struct ZXErrStack {
  char const *infos[ZX_ERR_STACK_CAP_];
  zx_u32 depth;
  zx_u16 info_lengths[ZX_ERR_STACK_CAP_];
} ZXErrStack;

static inline ZXErrStack ZXErrStack_init() {
  return (ZXErrStack){
      .infos = {0},
      .info_lengths = {0},
      .depth = 0wbu,
  };
}

static inline bool zx_err_try_(ZXStatus status, char const *info,
                               zx_usize info_len, ZXErrStack *stack) {
  if (status != ZXStatus_Ok) {
    zx_u32 const depth = stack->depth;
    zx_u32 const write_idx =
        (depth == ZX_ERR_STACK_CAP_) ? (ZX_ERR_STACK_CAP_ - 1) : depth;
    stack->info_lengths[write_idx] = (zx_u16)info_len;
    stack->infos[write_idx] = info;
    stack->depth = write_idx + 1;
    return true;
  } else {
    return false;
  }
}

static inline void zx_err_return_(char const *info, zx_usize info_len,
                                  ZXErrStack *stack) {
  stack->info_lengths[0] = (zx_u16)info_len;
  stack->infos[0] = info;
  stack->depth = 1;
}

static inline bool zx_err_assert_(bool success, char const *info,
                                  zx_usize info_len, ZXErrStack *stack) {
  if (!success) {
    stack->info_lengths[0] = (zx_u16)info_len;
    stack->infos[0] = info;
    stack->depth = 1;
  }
  return success;
}

static inline ZXStatus zx_err_convert_sys_status(zx_usize status) {
  switch (status) {
  case ZX_SYS_STATUS_SUCCESS: {
    return ZXStatus_Ok;
  }
  default: {
    return ZXStatus_SysErrUnexpected;
  }
  }
}

#endif

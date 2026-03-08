#ifndef ZX_ERR_H
#define ZX_ERR_H

#include <zx/zx_branch.h>
#include <zx/zx_def.h>
#include <zx/zx_fuzz_instr.h>

#define ZX_ERR_TRY(expr, stack_var)                                            \
  do {                                                                         \
    ZXStatus const zx_err_try_status_ = (expr);                                \
    if (zx_unlikely(zx_err_try_status_ != ZXStatus_Ok)) {                      \
      zx_err_stack_append_(                                                    \
          (ZXErrStackEntry_){.file_name = __FILE_NAME__,                       \
                             .file_name_len = __builtin_strlen(__FILE_NAME__), \
                             .line_no = __LINE__},                             \
          (stack_var));                                                        \
      ZX_FUZZ_ERR_MARKER;                                                      \
      return zx_err_try_status_;                                               \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

#define ZX_ERR_TRY_FATAL(expr, stack_var)                                      \
  do {                                                                         \
    ZXStatus const zx_err_try_fatal_status_ = (expr);                          \
    ZXErrStack *zx_err_try_fatal_stack_ = (stack_var);                         \
    if (zx_unlikely(zx_err_try_fatal_status_ != ZXStatus_Ok)) {                \
      zx_err_stack_append_(                                                    \
          (ZXErrStackEntry_){.file_name = __FILE_NAME__,                       \
                             .file_name_len = __builtin_strlen(__FILE_NAME__), \
                             .line_no = __LINE__},                             \
          zx_err_try_fatal_stack_);                                            \
      ZX_FUZZ_ABORT_MARKER;                                                    \
      zx_err_abort_(zx_err_try_fatal_stack_);                                  \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

#define ZX_ERR_RETURN(expr, stack_var)                                         \
  do {                                                                         \
    ZXStatus const zx_err_return_status_ = (expr);                             \
    ZXErrStack *zx_err_return_stack_ = (stack_var);                            \
    zx_err_return_stack_->depth = 0;                                           \
    zx_err_stack_append_(                                                      \
        (ZXErrStackEntry_){.file_name = __FILE_NAME__,                         \
                           .file_name_len = __builtin_strlen(__FILE_NAME__),   \
                           .line_no = __LINE__},                               \
        zx_err_return_stack_);                                                 \
    ZX_FUZZ_ERR_MARKER;                                                        \
    return zx_err_return_status_;                                              \
  } while (false)

#define ZX_ERR_ASSERT(expr, stack_var)                                         \
  do {                                                                         \
    bool const zx_err_assert_success_ = (expr);                                \
    if (zx_unlikely(!zx_err_assert_success_)) {                                \
      ZXErrStack *zx_err_assert_stack_ = (stack_var);                          \
      zx_err_assert_stack_->depth = 0;                                         \
      zx_err_stack_append_(                                                    \
          (ZXErrStackEntry_){.file_name = __FILE_NAME__,                       \
                             .file_name_len = __builtin_strlen(__FILE_NAME__), \
                             .line_no = __LINE__},                             \
          zx_err_assert_stack_);                                               \
      ZX_FUZZ_ERR_MARKER;                                                      \
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

constexpr zx_u16 ZX_ERR_STACK_CAP_ = 64;

typedef struct ZXErrStackEntry_ {
  char const *file_name;
  zx_u16 line_no;
  zx_u8 file_name_len;
  zx_u8 padding_[5];
} ZXErrStackEntry_;
static_assert(sizeof(ZXErrStackEntry_) == 2 * sizeof(void *));

typedef struct ZXErrStack {
  zx_u16 depth;
  ZXErrStackEntry_ entries[ZX_ERR_STACK_CAP_];
} ZXErrStack;
static_assert(sizeof(ZXErrStack) ==
              sizeof(ZXErrStackEntry_) * ZX_ERR_STACK_CAP_ + sizeof(void *));
// make sure it is small enough since it is expected to be allocated on stack
static_assert(sizeof(ZXErrStack) == 1032);

static inline ZXErrStack ZXErrStack_init() { return (ZXErrStack){0}; }

static inline void zx_err_stack_append_(ZXErrStackEntry_ entry,
                                        ZXErrStack *stack) {
  constexpr zx_u16 MAX_WRITE_IDX = ZX_ERR_STACK_CAP_ - 1;

  zx_u16 const depth = stack->depth;
  zx_u16 const write_idx = (depth < MAX_WRITE_IDX) ? depth : MAX_WRITE_IDX;
  stack->entries[write_idx] = entry;
  stack->depth = write_idx + 1;
}

// Formats the error stack to stderr and aborts the program
[[noreturn]] void zx_err_abort_(ZXErrStack const *err_stack);

#endif

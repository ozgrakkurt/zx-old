#ifndef ZX_ERR_H
#define ZX_ERR_H

#include <zx/zx_int.h>

// just push the location into the error stack, don't check or return any
// statuses. Intended to be used in fuctions that are fallible but have to do
// some cleanup before returning.
#define ZX_ERR_APPEND_LOC(stack_var)                                           \
  do {                                                                         \
    ZXErrStack *zx_err_stack_ = (stack_var);                                   \
    zx_err_stack_->append_(                                                    \
        ZXErrStackEntry_{.file_name = __FILE_NAME__,                           \
                         .line_no = __LINE__,                                  \
                         .file_name_len = __builtin_strlen(__FILE_NAME__)});   \
  } while (false)

// checks if expr evaluates to an error status.
// appends to stack and returns if the status is an error.
// Intended for propagating errors when calling fallible functions.
#define ZX_ERR_TRY(expr, stack_var)                                            \
  do {                                                                         \
    ZXStatus const zx_status_ = (expr);                                        \
    if (zx_status_ != ZXStatus::Ok) [[unlikely]] {                             \
      ZXErrStack *zx_err_stack_ = (stack_var);                                 \
      zx_err_stack_->append_(                                                  \
          ZXErrStackEntry_{.file_name = __FILE_NAME__,                         \
                           .line_no = __LINE__,                                \
                           .file_name_len = __builtin_strlen(__FILE_NAME__)}); \
      ZX_FUZZ_ERR_MARKER;                                                      \
      return zx_status_;                                                       \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

// checks if expr evaluates to an error status.
// prints stack trace to stderr and aborts the program if the status is an
// error. Intended for handling errors in the "main" function of a program. or
// asserting out infallible errors.
#define ZX_ERR_TRY_FATAL(expr, stack_var)                                      \
  do {                                                                         \
    ZXStatus const zx_status_ = (expr);                                        \
    if (zx_status_ != ZXStatus::Ok) [[unlikely]] {                             \
      ZXErrStack *zx_err_stack_ = (stack_var);                                 \
      zx_err_stack_->append_(                                                  \
          ZXErrStackEntry_{.file_name = __FILE_NAME__,                         \
                           .line_no = __LINE__,                                \
                           .file_name_len = __builtin_strlen(__FILE_NAME__)}); \
      ZX_FUZZ_ABORT_MARKER;                                                    \
      zx_err_stack_->abort_();                                                 \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

// pushes the given error status into the stack and returns.
// Intended for returning error from a leaf function.
#define ZX_ERR_RETURN(expr, stack_var)                                         \
  do {                                                                         \
    ZXErrStack *zx_err_stack_ = (stack_var);                                   \
    ZXStatus const zx_status_ = (expr);                                        \
    zx_err_stack_->depth = 0;                                                  \
    zx_err_stack_->append_(                                                    \
        ZXErrStackEntry_{.file_name = __FILE_NAME__,                           \
                         .line_no = __LINE__,                                  \
                         .file_name_len = __builtin_strlen(__FILE_NAME__)});   \
    ZX_FUZZ_ERR_MARKER;                                                        \
    return zx_status_;                                                         \
  } while (false)

// asserts that given expr evaluates to true.
// Pushes location to error stack and returns assertion error if expr evaluates
// to false.
#define ZX_ERR_ASSERT(expr, stack_var)                                         \
  do {                                                                         \
    if (!(expr)) [[unlikely]] {                                                \
      ZX_ERR_RETURN(ZXStatus::AssertFail, stack_var);                          \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

constexpr zx_u16 ZX_ERR_STACK_CAP_ = zx_u16(64);

struct ZXErrStackEntry_ {
  char const *file_name;
  zx_u16 line_no;
  zx_u8 file_name_len;
};
static_assert(sizeof(ZXErrStackEntry_) == 2 * sizeof(void *));

// error stack for recording error propagation locations in a program.
// Can be used for printing stack traces.
struct ZXErrStack {
  zx_u16 depth;
  ZXErrStackEntry_ entries[ZX_ERR_STACK_CAP_];

  static inline ZXErrStack init() {
    ZXErrStack stack;
    stack.depth = 0;
    __builtin_memset(stack.entries, 0, sizeof(stack.entries));
    return stack;
  }

  inline void append_(ZXErrStackEntry_ entry) {
    constexpr zx_u16 MAX_WRITE_IDX = ZX_ERR_STACK_CAP_.sub_unsafe(1);

    zx_u16 const depth_ = this->depth;
    zx_u16 const write_idx = (depth_ < MAX_WRITE_IDX) ? depth_ : MAX_WRITE_IDX;
    this->entries[write_idx] = entry;
    this->depth = write_idx.add_unsafe(1);
  }

  // Formats the error stack to stderr and aborts the program
  [[noreturn]] void abort_() const;
};
static_assert(sizeof(ZXErrStack) ==
              sizeof(ZXErrStackEntry_) * ZX_ERR_STACK_CAP_ + sizeof(void *));
// make sure it is small enough since it is expected to be allocated on stack
static_assert(sizeof(ZXErrStack) == 1032);

enum class [[nodiscard]] ZXStatus : unsigned char {
  Ok = 0,
  UnexpectedError,
  AssertFail,
  OutOfMemory,
  ShortWrite,
  UnexpectedSysError,
};

#endif

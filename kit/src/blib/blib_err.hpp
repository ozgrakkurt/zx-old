#ifndef BLIB_ERR_HPP
#define BLIB_ERR_HPP

#include <cstdint>

#define BLIB_ERR_STRINGIFY(x) #x
#define BLIB_ERR_LINE_TO_STRING(x) BLIB_ERR_STRINGIFY(x)
#define BLIB_ERR_LOC __FILE__ ":_line_" BLIB_ERR_LINE_TO_STRING(__LINE__)

#define BLIB_ERR_TRY(expr)                                            \
  do {                                                                \
    blib::Status blib_err_status_ = (expr);                           \
    if (blib_err_status_ != blib::Status::Ok) {                       \
      blib::err::stack_trace[blib::err::stack_depth] = BLIB_ERR_LOC;  \
      blib::err::stack_depth =                                        \
          (blib::err::stack_depth + 1) & blib::err::STACK_DEPTH_MASK; \
      return blib_err_status_;                                        \
    }                                                                 \
  } while (0)

#define BLIB_ERR_ASSERT(expr) BLIB_ERR_TRY(blib::err_assert((expr)));

namespace blib {

namespace err {

static uint32_t constexpr MAX_STACK_DEPTH = 512;
static uint32_t constexpr STACK_DEPTH_MASK = MAX_STACK_DEPTH - 1;

extern _Thread_local char const* stack_trace[MAX_STACK_DEPTH];
extern _Thread_local uint32_t stack_depth;

}  // namespace err

enum class [[nodiscard]] Status : uint8_t {
  Ok = 0,
  UnexpectedError = 1,
  AssertFail = 2,
  OutOfMemory = 3,
};

inline char const* status_as_str(Status status) {
  switch (status) {
    case Status::Ok:
      return "Status::Ok";
    case Status::UnexpectedError:
      return "Status::UnexpectedError";
    case Status::AssertFail:
      return "Status::AssertFail";
    case Status::OutOfMemory:
      return "Status::OutOfMemory";
    default:
      return "Status::Unknown";
  }
}

inline Status err_assert(bool cond) {
  if (cond) {
    return Status::Ok;
  } else {
    return Status::AssertFail;
  }
}

}  // namespace blib

#endif

#include <blib_err.hpp>

namespace blib {

namespace err {

_Thread_local char const* stack_trace[MAX_STACK_DEPTH];
_Thread_local uint32_t stack_depth = 0;

}  // namespace err

}  // namespace blib

#ifndef BLIB_STACK_TRACE_HPP
#define BLIB_STACK_TRACE_HPP

#include <cstddef>

#include "./blib_alloc.hpp"
#include "./blib_err.hpp"

#define BLIB_STACK_TRACE_UNWRAP(expr)                                 \
  do {                                                                \
    blib::Status blib_err_status_ = (expr);                           \
    if (blib_err_status_ != blib::Status::Ok) {                       \
      blib::err::stack_trace[blib::err::stack_depth] = BLIB_ERR_LOC;  \
      blib::err::stack_depth =                                        \
          (blib::err::stack_depth + 1) & blib::err::STACK_DEPTH_MASK; \
      blib::stack_trace_panic(blib_err_status_);                      \
    }                                                                 \
  } while (0)

namespace blib {

void stack_trace_panic(Status status);

Status stack_trace_collect(Alloc* alloc, char const** trace, size_t* trace_len);

}  // namespace blib

#endif

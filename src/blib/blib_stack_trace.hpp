#ifndef BLIB_STACK_TRACE_HPP
#define BLIB_STACK_TRACE_HPP

#include <cstddef>

#include "./blib_alloc.hpp"
#include "./blib_err.hpp"

namespace blib {

Status stack_trace_collect(char* buf, size_t capacity);

}  // namespace blib

#endif

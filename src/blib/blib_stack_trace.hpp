#ifndef BLIB_STACK_TRACE_HPP
#define BLIB_STACK_TRACE_HPP

#include <cstddef>

#include "./blib_alloc.hpp"
#include "./blib_err.hpp"

namespace blib {

Status stack_trace_collect(char* buf, size_t capacity) {
  size_t offset = 0;

  for (uint32_t i = 0; i < blib::err::stack_depth; i++) {
    char const* err_str = blib::err::stack_trace[i];

    size_t const len = strlen(err_str);

    if (offset + len + 1 > capacity) {
      BLIB_ERR_START(Status::OutOfMemory);
    }

    memcpy(&buf[offset], err_str, len);
    buf[offset + len] = '\n';

    offset += (len + 1);
  }

  buf[offset] = 0;
  return Status::Ok;
}

}  // namespace blib

#endif

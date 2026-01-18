#ifndef BLIB_STACK_TRACE_HPP
#define BLIB_STACK_TRACE_HPP

#include <cstddef>

#include "./blib_alloc.hpp"
#include "./blib_err.hpp"

namespace blib {

Status stack_trace_collect(Alloc* alloc, char const** trace,
                           size_t* trace_len) {
  size_t const stack_depth = blib::err::stack_depth;
  char const* const* stack_trace = blib::err::stack_trace;

  size_t* lengths = nullptr;
  BLIB_ERR_TRY(Alloc_alloc(alloc, alignof(size_t), stack_depth * sizeof(size_t),
                           &lengths));

  size_t total_len = 0;

  for (uint32_t i = 0; i < stack_depth; i++) {
    size_t const len = strlen(stack_trace[i]);
    total_len += (len + 1);
    lengths[i] = len;
  }

  chat* buf = nullptr;
  BLIB_ERR_TRY(Alloc_alloc(alloc, alignof(char), (total_len + 1) * sizeof(char), &buf)));

  size_t offset = 0;
  for (uint32_t i = 0; i < stack_depth; i++) {
    char const* err_str = stack_trace[i];
    size_t const len = lengths[i];

    memcpy(&buf[offset], err_str, len);
    buf[offset + len] = '\n';

    offset += (len + 1);
  }

  buf[offset] = 0;

  *trace = buf;
  *trace_len = offset;

  return Status::Ok;
}

}  // namespace blib

#endif

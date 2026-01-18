#include "./blib_stack_trace.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "./blib_alloc.hpp"

namespace blib {

Status stack_trace_collect(Status status, Alloc* alloc, char const** trace,
                           size_t* trace_len) {
  size_t const stack_depth = blib::err::stack_depth;
  char const* const* stack_trace = blib::err::stack_trace;

  size_t* lengths = nullptr;
  BLIB_ERR_TRY(
      (Alloc_alloc(alloc, alignof(size_t), stack_depth * sizeof(size_t),
                   (uint8_t**)&lengths)));

  size_t total_len = 0;

  for (uint32_t i = 0; i < stack_depth; i++) {
    size_t const len = strlen(stack_trace[i]);
    total_len += (len + 2);
    lengths[i] = len;
  }

  char const* status_str = status_as_str(status);
  size_t const status_str_len = strlen(status_str);
  char const* prefix = "Failed with error: ";
  size_t const prefix_len = strlen(prefix);

  size_t offset = status_str_len + prefix_len + 1;

  char* buf = nullptr;
  BLIB_ERR_TRY(
      (Alloc_alloc(alloc, alignof(char),
                   (total_len + offset + 1) * sizeof(char), (uint8_t**)&buf)));

  memcpy(buf, prefix, prefix_len);
  memcpy(&buf[prefix_len], status_str, status_str_len);
  buf[prefix_len + status_str_len] = '\n';

  for (uint32_t i = 0; i < stack_depth; i++) {
    char const* err_str = stack_trace[i];
    size_t const len = lengths[i];

    buf[offset] = '\t';
    memcpy(&buf[offset + 1], err_str, len);
    buf[offset + 1 + len] = '\n';

    offset += (len + 2);
  }

  buf[offset] = 0;

  assert(offset == total_len + status_str_len + prefix_len + 1);

  *trace = buf;
  *trace_len = offset;

  return Status::Ok;
}

void stack_trace_panic(Status status) {
  size_t constexpr capacity = 1 << 17;

  uint8_t* buf = (uint8_t*)malloc(capacity);
  if (buf == nullptr) {
    fprintf(stderr, "failed to allocate buffer for stack trace, aborting.\n");
    abort();
  }

  Alloc alloc = Alloc_init(buf, capacity);

  char const* trace = "";
  size_t trace_len = 0;

  Status const collect_status =
      stack_trace_collect(status, &alloc, &trace, &trace_len);
  if (collect_status != Status::Ok) {
    fprintf(stderr,
            "failed to collect stack trace for status %s with status: "
            "%s.\naborting.\n",
            status_as_str(status), status_as_str(collect_status));
    abort();
  }

  fprintf(stderr, "%saborting program.\n", trace);

  abort();
}

}  // namespace blib

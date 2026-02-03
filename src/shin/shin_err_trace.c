#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>

void shin_err_trace_panic(ShinStatus status);

// Outputs a string to trace and the length of the string to trace_len
ShinStatus shin_err_trace_collect(ShinAlloc* alloc, ShinErrStack const* stack,
                                  char const** trace, size_t* trace_len) {
  ShinErrStack const stack_state = *stack;

  shin_usize* lengths = nullptr;
  BLIB_ERR_TRY((alloc->alloc<size_t>(stack_depth, &lengths)));

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
  BLIB_ERR_TRY((alloc->alloc<char>(total_len + offset + 1, &buf)));

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

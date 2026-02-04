#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <stddef.h>
#include <string.h>

ShinStatus shin_err_trace_collect(ShinAlloc* alloc, ShinErrStack const* stack,
                                  char const** trace, shin_usize* trace_len,
                                  ShinErrStack* err_stack) {
  shin_usize const depth = stack->depth;
  char const* const* infos = stack->infos;
  shin_usize const* info_lengths = stack->info_lengths;

  shin_usize total_len = 0;
  for (shin_u32 i = 0; i < depth; i++) {
    total_len += info_lengths[i];
  }

  constexpr char prefix[] = "Error Trace: \n";
  constexpr shin_usize prefix_len = sizeof(prefix) - 1;

  shin_usize const alloc_size = prefix_len + total_len + 1;

  char* buf = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, alloc_size, (void**)&buf, err_stack)),
               err_stack);

  memcpy(buf, prefix, prefix_len);

  shin_usize offset = prefix_len;
  for (shin_u32 i = 0; i < depth; i++) {
    shin_usize const info_len = info_lengths[i];
    char const* info = infos[i];
    memcpy(&buf[offset], info, info_len);
    offset += info_len;
  }

  buf[offset] = '\0';

  SHIN_DBG_ASSERT((offset + 1 == alloc_size));

  *trace = buf;
  *trace_len = offset;

  return ShinStatus_Ok;
}

// Outputs the trace to stderr and aborts the program
[[noreturn]] void shin_err_trace_abort(ShinErrStack const* stack) {
  SHIN_ASSERT((stack != nullptr));

  if (0 > fprintf(stderr, "Aborting with stack trace:\n")) {
    abort();
  }

  for (shin_u32 i = 0; i < stack->depth; i++) {
    if (0 > fprintf(stderr, "%s", stack->infos[i])) {
      abort();
    }
  }

  abort();
}

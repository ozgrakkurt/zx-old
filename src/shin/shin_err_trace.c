#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

[[noreturn]] void shin_err_trace_panic(ShinStatus status) {
  (void)status;
  abort();
}

ShinStatus shin_err_trace_collect(ShinAlloc* alloc, ShinErrStack const* stack,
                                  char const** trace, size_t* trace_len,
                                  ShinErrStack* err_stack) {
  ShinErrStack const stack_state = *stack;

  shin_usize total_len = 0;
  for (shin_u32 i = 0; i < stack_state.depth; i++) {
    total_len += stack_state.info_lengths[i];
  }

  constexpr char prefix[] = "Error Trace: \n";
  constexpr shin_usize prefix_len = sizeof(prefix) - 1;

  shin_usize const alloc_size = prefix_len + total_len + 1;

  char* buf = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, alloc_size, (void**)&buf, err_stack)),
               err_stack);

  memcpy(buf, prefix, prefix_len);

  shin_usize offset = prefix_len;
  for (shin_u32 i = 0; i < stack_state.depth; i++) {
    shin_usize const info_len = stack_state.info_lengths[i];
    char const* info = stack_state.infos[i];
    memcpy(&buf[offset], info, info_len);
    offset += info_len;
  }

  buf[offset] = '\0';

  SHIN_DBG_ASSERT((offset + 1 == alloc_size));

  *trace = buf;
  *trace_len = offset;

  return ShinStatus_Ok;
}

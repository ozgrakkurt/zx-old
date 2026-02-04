#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>

ShinStatus err_func(shin_usize depth, ShinErrStack* err_stack) {
  if (depth > 128) {
    SHIN_ERR_RETURN((ShinStatus_UnexpectedError), err_stack);
  }
  if (depth > 64) {
    SHIN_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else if (depth > 32) {
    SHIN_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else if (depth > 16) {
    SHIN_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  } else {
    SHIN_ERR_TRY((err_func(depth + 1, err_stack)), err_stack);
  }
  return ShinStatus_Ok;
}

ShinStatus test(ShinAlloc* alloc, ShinErrStack* err_stack) {
  (void)alloc;

  SHIN_ERR_TRY((err_func(1, err_stack)), err_stack);

  return ShinStatus_Ok;
}

constexpr shin_usize MEMORY_SIZE = 1 << 16;

int main(void) {
  void* const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  ShinStatus const status = test(&alloc, &err_stack);

  if (status != ShinStatus_Ok) {
    char const* trace = nullptr;
    shin_usize trace_size = 0;

    // secondary error stack for tracking errors happening while collecting the
    // error trace
    ShinErrStack collect_stack = ShinErrStack_init();
    SHIN_ERR_TRACE_ABORT((shin_err_trace_collect(&alloc, &err_stack, &trace,
                                                 &trace_size, &collect_stack)),
                         &collect_stack);

    // fprintf(stderr, "Collected trace:\n%s", trace);
  }

  // SHIN_DBG_ASSERT(false);
  // SHIN_ASSERT(false);
  // shin_page_alloc_free(nullptr, MEMORY_SIZE);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

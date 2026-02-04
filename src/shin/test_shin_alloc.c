#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>

ShinStatus test(ShinAlloc* alloc, ShinErrStack* err_stack) {
  (void)alloc;

  SHIN_ERR_RETURN((ShinStatus_UnexpectedError), err_stack);

  return ShinStatus_Ok;
}

constexpr shin_usize MEMORY_SIZE = 1 << 21;

int main(void) {
  void* const page = shin_page_alloc_thp_2mb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((test(&alloc, &err_stack)), &err_stack);

  // SHIN_DBG_ASSERT(false);
  // SHIN_ASSERT(false);
  // shin_page_alloc_free(nullptr, MEMORY_SIZE);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

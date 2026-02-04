#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>

constexpr shin_usize MEMORY_SIZE = 1 << 16;

int main(void) {
  void* const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  (void)err_stack;
  (void)alloc;

  // SHIN_DBG_ASSERT(false);
  // SHIN_ASSERT(false);
  // shin_page_alloc_free(nullptr, MEMORY_SIZE);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

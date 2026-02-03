#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>

// ShinStatus test(void) {}

constexpr shin_usize MEMORY_SIZE = 1 << 17;

int main(void) {
  void* const page = shin_page_alloc_4kb(MEMORY_SIZE);

  (void)page;

  SHIN_DBG_ASSERT(false);
  SHIN_ASSERT(false);

  return 0;
}

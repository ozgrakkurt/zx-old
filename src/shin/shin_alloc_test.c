#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <stdio.h>
#include <stdlib.h>

constexpr shin_usize MEMORY_SIZE = 1 << 16;

ShinStatus test_allocator(ShinAlloc* alloc, ShinErrStack* err_stack) {
  SHIN_ERR_TRY((ShinAlloc_reset(alloc, err_stack)), err_stack);

  shin_i32** slice = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, 100 * sizeof(shin_i32*), (void**)&slice,
                                err_stack)),
               err_stack);
  SHIN_ERR_ASSERT((slice != nullptr), err_stack);
  SHIN_ERR_ASSERT((((shin_usize)slice) % alignof(shin_i32*) == 0), err_stack);

  for (shin_usize i = 0; i < 100; i++) {
    slice[i] = nullptr;
    SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(shin_i32), (void**)&slice[i],
                                  err_stack)),
                 err_stack);
    SHIN_ERR_ASSERT((slice[i] != nullptr), err_stack);
    SHIN_ERR_ASSERT((((shin_usize)slice[i]) % alignof(shin_i32) == 0),
                    err_stack);
    *(slice[i]) = (shin_i32)i;
  }

  for (shin_usize i = 0; i < 100; i++) {
    SHIN_ERR_ASSERT((*(slice[i]) == (shin_i32)i), err_stack);
  }

  shin_u8* zero_ptr = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, 0, (void**)&zero_ptr, err_stack)),
               err_stack);
  SHIN_ERR_ASSERT((zero_ptr != nullptr), err_stack);

  shin_u8* big_ptr = nullptr;
  ShinStatus const oom_status =
      ShinAlloc_alloc(alloc, MEMORY_SIZE, (void**)&big_ptr, err_stack);
  SHIN_ERR_ASSERT((oom_status == ShinStatus_OutOfMemory), err_stack);
  SHIN_ERR_ASSERT((big_ptr == nullptr), err_stack);

  SHIN_ERR_TRY((ShinAlloc_reset(alloc, err_stack)), err_stack);
  SHIN_ERR_TRY(
      (ShinAlloc_alloc(alloc, MEMORY_SIZE, (void**)&big_ptr, err_stack)),
      err_stack);
  SHIN_ERR_ASSERT((big_ptr != nullptr), err_stack);

  zero_ptr = nullptr;
  ShinStatus const oom2 =
      ShinAlloc_alloc(alloc, 1, (void**)&zero_ptr, err_stack);
  SHIN_ERR_ASSERT((oom2 == ShinStatus_OutOfMemory), err_stack);
  SHIN_ERR_ASSERT((zero_ptr == nullptr), err_stack);

  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, 0, (void**)&zero_ptr, err_stack)),
               err_stack);
  SHIN_ERR_ASSERT((zero_ptr != nullptr), err_stack);

  return ShinStatus_Ok;
}

ShinStatus test_allocator_aligned(ShinAlloc* alloc, ShinErrStack* err_stack) {
  SHIN_ERR_TRY((ShinAlloc_reset(alloc, err_stack)), err_stack);

  shin_usize const alignments[] = {1, 2, 4, 8, 16, 32, 64};

  shin_u8* ptr = nullptr;

  for (shin_usize i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
    shin_usize const alignment = alignments[i];

    ptr = nullptr;
    SHIN_ERR_TRY((ShinAlloc_aligned_alloc(alloc, alignment, 69, (void**)&ptr,
                                          err_stack)),
                 err_stack);
    SHIN_ERR_ASSERT((ptr != nullptr), err_stack);
    SHIN_ERR_ASSERT((((shin_usize)ptr) % alignment == 0), err_stack);
    SHIN_ERR_ASSERT((((shin_usize)ptr) % alignof(max_align_t) == 0), err_stack);
  }

  return ShinStatus_Ok;
}

int main(void) {
  void* const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((test_allocator(&alloc, &err_stack)), &err_stack);
  SHIN_ERR_TRACE_ABORT((test_allocator_aligned(&alloc, &err_stack)),
                       &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

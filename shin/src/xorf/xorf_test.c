#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_xoshiro256.h>
#include <string.h>
#include <xorf/xorf.h>

constexpr shin_usize MEMORY_SIZE = 1 << 20;

static ShinStatus smoke_test(ShinAlloc *alloc, ShinAlloc *scratch_alloc,
                             shin_u32 const len, ShinErrStack *err_stack) {
  shin_u64 *keys = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(shin_u64) * len, (void **)&keys,
                                err_stack)),
               err_stack);

  ShinXoshiro256 rand = ShinXoshiro256_init(0);
  ShinXoshiro256_fill(&rand, (shin_u8 *)keys, sizeof(shin_u64) * len);

  XorfHeader header = {0};
  shin_u16 *fingerprints = nullptr;
  SHIN_ERR_TRY((xorf_construct(alloc, scratch_alloc, keys, len, &header,
                               &fingerprints, err_stack)),
               err_stack);

  for (shin_usize i = 0; i < len; i++) {
    shin_u64 const key = keys[i];
    SHIN_ERR_ASSERT((xorf_check(&header, fingerprints, key)), err_stack);
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();

  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE / 2);
  ShinAlloc scratch_alloc =
      ShinAlloc_init(&page[MEMORY_SIZE / 2], MEMORY_SIZE / 2);

  SHIN_ERR_TRACE_ABORT((smoke_test(&alloc, &scratch_alloc, 699, &err_stack)),
                       &err_stack);
  SHIN_ERR_TRACE_ABORT((ShinAlloc_reset(&alloc, &err_stack)), &err_stack);
  SHIN_ERR_TRACE_ABORT((ShinAlloc_reset(&scratch_alloc, &err_stack)),
                       &err_stack);

  SHIN_ERR_TRACE_ABORT((smoke_test(&alloc, &scratch_alloc, 0, &err_stack)),
                       &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

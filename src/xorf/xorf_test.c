#include <xorf/xorf.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_usize MEMORY_SIZE = 1 << 20;

static ZXStatus smoke_test(ZXAlloc *alloc, ZXAlloc *scratch_alloc,
                           zx_u32 const len, ZXErrStack *err_stack) {
  zx_u64 *keys = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, sizeof(zx_u64) * len, (void **)&keys, err_stack)),
      err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);
  ZXXoshiro256_fill(&rand, (zx_u8 *)keys, sizeof(zx_u64) * len);

  XorfHeader header = {0};
  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((xorf_construct(alloc, scratch_alloc, keys, len, &header,
                             &fingerprints, err_stack)),
             err_stack);

  for (zx_usize i = 0; i < len; i++) {
    zx_u64 const key = keys[i];
    ZX_ERR_ASSERT((xorf_check(&header, fingerprints, key)), err_stack);
  }

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_4kb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();

  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE / 2);
  ZXAlloc scratch_alloc = ZXAlloc_init(&page[MEMORY_SIZE / 2], MEMORY_SIZE / 2);

  ZX_ERR_TRACE_TRY_ABORT((smoke_test(&alloc, &scratch_alloc, 699, &err_stack)),
                         &err_stack);
  ZXAlloc_reset(&alloc);
  ZXAlloc_reset(&scratch_alloc);

  ZX_ERR_TRACE_TRY_ABORT((smoke_test(&alloc, &scratch_alloc, 0, &err_stack)),
                         &err_stack);

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

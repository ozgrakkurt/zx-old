#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_queue.h>
#include <zx/zx_rapidhash_v3.h>

typedef struct FuzzCtx {
  void *mem;
  zx_u32 cap;
} FuzzCtx;

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;
  ZXAlloc alloc = ZXAlloc_init(ctx->mem, ctx->cap);

  zx_u32 const MAX_CAP = 256;
  zx_u32 const capacity = ((zx_u32)ZXFuzzInput_next(fuzz_input)) % MAX_CAP;

  zx_u32 const MAX_N_OPS = 1024;
  zx_u32 const n_ops = ((zx_u32)ZXFuzzInput_next(fuzz_input)) % MAX_N_OPS;

  ZXQueue queue;
  ZX_ERR_TRY((ZXQueue_init(&alloc, capacity, alignof(zx_u32), sizeof(zx_u32),
                           &queue, err_stack)),
             err_stack);

  zx_u32 start = 0;
  zx_u32 end = 0;

  for (zx_u32 i = 0; i < n_ops; i++) {
    ZX_ERR_ASSERT((zx_chk_sub_u32(capacity, ZXQueue_capacity_left(&queue)) ==
                   (zx_chk_sub_u32(end, start))),
                  err_stack);

    zx_u8 const op = (zx_u8)ZXFuzzInput_next(fuzz_input) % 2;

    switch (op) {
    // push
    case 0: {
      bool const success = ZXQueue_push(&queue, &end, sizeof(zx_u32));
      if (success) {
        end += 1;
      }

      break;
    }
    // pop
    case 1: {
      zx_u32 v = ZX_U32_MAX;
      bool const success = ZXQueue_pop(&queue, sizeof(zx_u32), &v);
      if (success) {
        ZX_ASSERT((v == start));
        start += 1;
      }

      break;
    }
    default: {
      ZX_ASSERT((false));
      break;
    }
    }
  }

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_u64 MEM_CAP = (zx_u64){1} << 30;
  constexpr zx_u64 FUZZ_MEM_CAP = (zx_u64){1} << 29;

  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEM_CAP, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  zx_u8 *fuzz_mem = nullptr;
  ZX_ERR_TRY_FATAL(ZXAlloc_alloc(&alloc, alignof(zx_u8), FUZZ_MEM_CAP,
                                 &fuzz_mem, &err_stack),
                   &err_stack);

  FuzzCtx const ctx = {
      .mem = fuzz_mem,
      .cap = FUZZ_MEM_CAP,
  };

  char const *tgt_file_filter = "zx_queue";

  ZXFuzzParams const params = {
      .usr_ctx = (void *)&ctx,
      .fuzz_one = fuzz_one,
      .tgt_file_filter = tgt_file_filter,
      .tgt_file_filter_len = (zx_u16)__builtin_strlen(tgt_file_filter),
  };

  // initiate main loop
  ZX_ERR_TRY_FATAL((zx_fuzz_main(&alloc, &params, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEM_CAP, &err_stack)), &err_stack);

  return 0;
}

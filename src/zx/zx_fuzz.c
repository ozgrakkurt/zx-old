#ifdef ZX_FUZZING_ENABLED

#include "./zx_fuzz_linker_set.h"
#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_sys.h>
#include <zx/zx_xoshiro256.h>

static ZXStatus zx_fuzz_print_counts(ZXFuzzLinkerSet const *state,
                                     ZXFmtBuf *fmt_buf, ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "Found:\n\t MARKERS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, state->n_markers, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ERRS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, state->n_errs, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\t ABORTS: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(fmt_buf, state->n_aborts, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(fmt_buf, "\n\n", err_stack)), err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(fmt_buf, ZX_STDERR, err_stack)), err_stack);

  ZXFmtBuf_reset(fmt_buf);

  return ZXStatus_Ok;
}

static void zx_fuzz_clear_state(ZXFuzzLinkerSet const *state) {
  __builtin_memset(state->marker_hit, 0, state->n_markers);
  __builtin_memset(state->err_hit, 0, state->n_errs);
}

static zx_u64 zx_fuzz_calculate_score(ZXFuzzLinkerSet const *state) {
  zx_u64 score = 0;

  for (zx_usize i = 0; i < state->n_markers; i++) {
    if (state->marker_hit[i] > 0) {
      score++;
    }
  }

  for (zx_usize i = 0; i < state->n_errs; i++) {
    if (state->err_hit[i] > 0) {
      score++;
    }
  }

  return score;
}

static zx_u64 zx_fuzz_calculate_signature(ZXFuzzLinkerSet const *state) {
  zx_u64 const marker_hash =
      zx_rapidhash_v3(state->marker_hit, state->n_markers, 0);
  zx_u64 const err_hash = zx_rapidhash_v3(state->err_hit, state->n_errs, 0);
  return zx_rapidhash_v3((zx_u8 const *)&marker_hash, sizeof(zx_u64), err_hash);
}

typedef struct ZXFuzzRunResult {
  zx_u64 score;
  zx_u64 signature;
  zx_u32 consumed_count;
} ZXFuzzRunResult;

static ZXStatus zx_fuzz_do_one_run(ZXFuzzLinkerSet const *state, void *usr_ctx,
                                   zx_u64 *fuzz_input, zx_u32 fuzz_input_len,
                                   ZXFuzzRunResult *result,
                                   ZXErrStack *err_stack) {
  ZXFuzzInput input = ZXFuzzInput_init(fuzz_input, fuzz_input_len);

  zx_fuzz_clear_state(state);

  ZX_ERR_TRY((zx_fuzz_one(usr_ctx, &input, err_stack)), err_stack);

  zx_u64 const signature = zx_fuzz_calculate_signature(state);
  zx_u64 const score = zx_fuzz_calculate_score(state);

  *result = (ZXFuzzRunResult){
      .score = score,
      .signature = signature,
      .consumed_count = ZXFuzzInput_consumed_count(&input),
  };

  return ZXStatus_Ok;
}

static ZXStatus zx_fuzz_main(ZXFuzzLinkerSet const *state, ZXFmtBuf *fmt_buf,
                             zx_u64 *fuzz_input, zx_u32 fuzz_input_len,
                             ZXErrStack *err_stack) {
  ZX_ERR_TRY((zx_fuzz_print_counts(state, fmt_buf, err_stack)), err_stack);

  void *usr_ctx = nullptr;
  ZX_ERR_TRACE_TRY_ABORT((zx_fuzz_setup(usr_ctx, err_stack)), err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);

  // random seed the input
  ZXXoshiro256_fill(&rand, (zx_u8 *)fuzz_input, fuzz_input_len * sizeof(zx_u64));

  // do a baseline run
  ZXFuzzRunResult baseline_result = {0};
  ZX_ERR_TRY((zx_fuzz_do_one_run(state, usr_ctx, fuzz_input, fuzz_input_len,
                                 &baseline_result, err_stack)),
             err_stack);
  while (true) {
    // select a random position to mutate
    zx_u32 const mut_idx =
        zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), fuzz_input_len);

    // try some different values in this position
    // change to a different position if we can't improve for some number of
    // iterations
    constexpr zx_u32 MAX_STALL = 1024;
    zx_u32 stall = 0;
    while (stall < MAX_STALL) {
      zx_u64 const new_val = ZXXoshiro256_next(&rand);
      fuzz_input[mut_idx] = new_val;

      ZXFuzzRunResult result = {0};
      ZX_ERR_TRY((zx_fuzz_do_one_run(state, usr_ctx, fuzz_input, fuzz_input_len,
                                     &result, err_stack)),
                 err_stack);

      if (result.score > baseline_result.score) {
        stall = 0;
      } else {
        stall += 1;
      }
    }
  }

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_usize MEM_CAP = 1 << 30;
  constexpr zx_u32 FMT_BUF_CAP = 1 << 14;
  constexpr zx_u32 INPUT_LEN = 1 << 19;

  ZXFuzzLinkerSet const state = ZXFuzzLinkerSet_load();

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  zx_u64 *fuzz_input = nullptr;
  ZX_ERR_TRACE_TRY_ABORT((ZXAlloc_alloc(&alloc, INPUT_LEN * sizeof(zx_u64),
                                        (void **)&fuzz_input, &err_stack)),
                         &err_stack);
  __builtin_memset(fuzz_input, 0, INPUT_LEN * sizeof(zx_u64));

  char *fmt_buf_alloc = nullptr;
  ZX_ERR_TRACE_TRY_ABORT(
      (ZXAlloc_alloc(&alloc, FMT_BUF_CAP, (void **)&fmt_buf_alloc, &err_stack)),
      &err_stack);
  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_alloc, FMT_BUF_CAP);

  // initiate main loop
  ZX_ERR_TRACE_TRY_ABORT(
      (zx_fuzz_main(&state, &fmt_buf, fuzz_input, INPUT_LEN, &err_stack)),
      &err_stack);

  zx_page_alloc_free(mem, MEM_CAP);
  return ZX_SYS_EXIT_SUCCESS;
}

#endif

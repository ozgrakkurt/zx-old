#ifdef ZX_FUZZING_ENABLED

#include "./zx_fuzz_inputs.h"
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

static ZXStatus zx_fuzz_print_input_reset_warning(ZXFmtBuf *fmt_buf,
                                                  ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      (ZXFmtBuf_format_cstr(
          fmt_buf,
          "WARNING: Resetting input history becuase it ran out of capacity.\n",
          err_stack)),
      err_stack);
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

int main(void) {
  constexpr zx_usize MEM_CAP = 1 << 30;
  constexpr zx_u32 FMT_BUF_CAP = 1 << 14;
  constexpr zx_u32 INPUT_CAP = 1 << 19;
  constexpr zx_u32 INPUT_INCREMENT_STEP = 1 << 9;
  constexpr zx_u64 STALL_RESELECT_THRESHOLD = 1 << 20;
  constexpr zx_u64 STALL_RANDOM_THRESHOLD = 1 << 25;

  constexpr zx_u32 INPUT_HISTORY_COUNT_CAP = 1 << 14;
  constexpr zx_u32 INPUT_HISTORY_SIZE_CAP = 1 << 24;

  ZXFuzzLinkerSet const state = ZXFuzzLinkerSet_load();

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  ZXXoshiro256 rand = ZXXoshiro256_init(0);

  zx_u8 *fuzz_input = nullptr;
  ZX_ERR_TRACE_TRY_ABORT(
      (ZXAlloc_alloc(&alloc, INPUT_CAP, (void **)&fuzz_input, &err_stack)),
      &err_stack);
  ZXXoshiro256_fill(&rand, fuzz_input, INPUT_CAP);
  zx_u32 fuzz_input_len = zx_int_min_u32(1 << 12, INPUT_CAP);

  char *fmt_buf_alloc = nullptr;
  ZX_ERR_TRACE_TRY_ABORT(
      (ZXAlloc_alloc(&alloc, FMT_BUF_CAP, (void **)&fmt_buf_alloc, &err_stack)),
      &err_stack);
  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_alloc, FMT_BUF_CAP);

  ZX_ERR_TRACE_TRY_ABORT((zx_fuzz_print_counts(&state, &fmt_buf, &err_stack)),
                         &err_stack);

  ZXFuzzInputs inputs;
  ZX_ERR_TRACE_TRY_ABORT(
      (ZXFuzzInputs_init(&alloc, INPUT_HISTORY_COUNT_CAP,
                         INPUT_HISTORY_SIZE_CAP, &inputs, &err_stack)),
      &err_stack);

  void *usr_ctx = nullptr;
  ZX_ERR_TRACE_TRY_ABORT((zx_fuzz_setup(&usr_ctx, &err_stack)), &err_stack);

  zx_u64 prev_score = 0;
  zx_u64 prev_best_score = 0;
  zx_u8 prev_byte = 0;
  zx_u64 reselect_stall = 0;
  zx_u64 random_stall = 0;
  zx_u32 mut_byte_idx =
      zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), fuzz_input_len);
  while (true) {
    zx_fuzz_clear_state(&state);

    zx_u32 n_consumed = 0;
    ZXStatus const fuzz_one_status = zx_fuzz_one(
        usr_ctx, fuzz_input, fuzz_input_len, &n_consumed, &err_stack);

    // do these before doing anything else so we don't taint the instrumentation
    // state.
    zx_u64 const signature = zx_fuzz_calculate_signature(&state);
    zx_u64 const score = zx_fuzz_calculate_score(&state);

    if (ZXStatus_Ok == fuzz_one_status) {
      // shrink so we don't save unused input tail
      // keep input size at least 1
      fuzz_input_len = n_consumed | (zx_u32){1wbu};
    }

    bool const resetted = ZXFuzzInputs_append_or_reset(
        &inputs, fuzz_input, fuzz_input_len, signature);
    if (resetted) {
      ZX_ERR_TRACE_TRY_ABORT(
          (zx_fuzz_print_input_reset_warning(&fmt_buf, &err_stack)),
          &err_stack);
    }

    // error handling
    if (ZXStatus_Ok != fuzz_one_status) {
      switch (fuzz_one_status) {
      case ZXStatus_FuzzInputTooShort: {
        ZX_ASSERT((fuzz_input_len < INPUT_CAP));
        zx_usize const max_inc = INPUT_CAP - fuzz_input_len;
        fuzz_input_len += zx_int_min_usize(max_inc, INPUT_INCREMENT_STEP);
        // continue main fuzzing loop
        continue;
        break;
      }
      default: {
        // this will abort for sure
        ZX_ERR_TRACE_TRY_ABORT((fuzz_one_status), &err_stack);
        // never will be reached
        ZX_ASSERT((false));
        break;
      }
      }
    }

    if (score > prev_best_score) {
      reselect_stall = 0;
      random_stall = 0;
      prev_best_score = score;
    } else {
      reselect_stall += 1;
      random_stall += 1;
    }

    if (score < prev_score) {
      fuzz_input[mut_byte_idx] = prev_byte;
    } else {
      prev_score = score;
    }

    mut_byte_idx =
        zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), fuzz_input_len);
    fuzz_input[mut_byte_idx] = (zx_u8)ZXXoshiro256_next(&rand);

    if (reselect_stall >= STALL_RESELECT_THRESHOLD &&
        ZXFuzzInputs_count(&inputs) > 0) {
      // reselect and old input
      zx_u8 const *src = nullptr;
      zx_u32 src_len = 0;
      ZXFuzzInputs_select_random(&inputs, &rand, &src, &src_len);
      ZX_ASSERT((src_len <= INPUT_CAP));
      __builtin_memcpy(fuzz_input, src, src_len);
      fuzz_input_len = src_len;

      // mutate random byte
      mut_byte_idx =
          zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand), fuzz_input_len);
      fuzz_input[mut_byte_idx] = (zx_u8)ZXXoshiro256_next(&rand);

      reselect_stall = 0;
    }

    if (random_stall >= STALL_RANDOM_THRESHOLD) {
      ZXXoshiro256_fill(&rand, fuzz_input, fuzz_input_len);
      random_stall = 0;
    }

    // conditionally do output printing for user
  }

  zx_page_alloc_free(mem, MEM_CAP);
  return ZX_SYS_EXIT_SUCCESS;
}

#endif

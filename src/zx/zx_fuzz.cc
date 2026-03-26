#ifdef ZX_FUZZING_ENABLED

#include "./zx_fuzz_linker_set.h"
#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_io.h>
#include <zx/zx_memmem.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_xoshiro256.h>

static void zx_fuzz_clear_linker_set(ZXFuzzLinkerSet const *linker_set) {
  __builtin_memset(linker_set->marker_hit, 0, linker_set->n_markers);
  __builtin_memset(linker_set->err_hit, 0, linker_set->n_errs);
}

static zx_u64 zx_fuzz_calculate_score(ZXFuzzLinkerSet const *linker_set) {
  zx_u64 score = 0;

  for (zx_u64 i = 0; i < linker_set->n_markers; i++) {
    if (linker_set->marker_hit[i] > 0) {
      score++;
    }
  }

  for (zx_u64 i = 0; i < linker_set->n_errs; i++) {
    if (linker_set->err_hit[i] > 0) {
      score++;
    }
  }

  return score;
}

static zx_u64 zx_fuzz_calculate_signature(ZXFuzzLinkerSet const *linker_set) {
  zx_u64 const marker_hash =
      zx_rapidhash_v3(linker_set->marker_hit, linker_set->n_markers, 0);
  zx_u64 const err_hash =
      zx_rapidhash_v3(linker_set->err_hit, linker_set->n_errs, 0);
  return zx_rapidhash_v3((zx_u8 const *)&marker_hash, sizeof(zx_u64), err_hash);
}

static zx_u32 zx_fuzz_count_tgt_coverage(bool const *is_tgt,
                                         bool const *coverage, zx_u32 n) {
  zx_u32 count = 0;
  for (zx_u32 i = 0; i < n; i++) {
    if (is_tgt[i] && coverage[i]) {
      count++;
    }
  }
  return count;
}

static zx_u32 zx_fuzz_count_coverage(bool const *coverage, zx_u32 n) {
  zx_u32 count = 0;
  for (zx_u32 i = 0; i < n; i++) {
    if (coverage[i]) {
      count++;
    }
  }
  return count;
}

// returns the number of new points reached
static zx_u32 zx_fuzz_update_coverage(ZXFuzzLinkerSet const *linker_set,
                                      bool *marker_coverage,
                                      bool *err_coverage) {
  zx_u32 new = 0;

  for (zx_u64 i = 0; i < linker_set->n_markers; i++) {
    bool const hit = linker_set->marker_hit[i] > 0;

    if (!marker_coverage[i] && hit) {
      new ++;
    }
    marker_coverage[i] |= hit;
  }

  for (zx_u64 i = 0; i < linker_set->n_errs; i++) {
    bool const hit = linker_set->err_hit[i] > 0;

    if (!err_coverage[i] && hit) {
      new ++;
    }
    err_coverage[i] |= hit;
  }

  return new;
}

constexpr zx_u32 ZX_FUZZ_VALUES_CAP = 512;
typedef struct ZXFuzzValues {
  zx_u64 values[ZX_FUZZ_VALUES_CAP];
  zx_u32 push_pos;
  zx_u32 n_filled;
} ZXFuzzValues;

static void ZXFuzzValues_push(ZXFuzzValues *queue, zx_u64 val) {
  constexpr zx_u32 PUSH_MASK = ZX_FUZZ_VALUES_CAP - 1;
  zx_u32 const push_pos = queue->push_pos & PUSH_MASK;
  queue->values[push_pos] = val;
  queue->push_pos = push_pos + 1;
  queue->n_filled = zx_int_min_u32(queue->n_filled + 1, ZX_FUZZ_VALUES_CAP);
}

// returns if the signature is "new" and inserts it into the deduplication map
static bool zx_fuzz_process_signature_is_new(zx_u32 *signature_dedup,
                                             zx_u32 signature_dedup_cap,
                                             zx_u64 signature) {
  zx_u32 const sig = (zx_u32)signature;
  zx_u32 const idx = zx_rand_mod_u32(sig, signature_dedup_cap);
  zx_u32 const prev = signature_dedup[idx];
  signature_dedup[idx] = sig;
  return prev != sig;
}

static zx_u32 zx_fuzz_count_tgt(char const *const *filenames,
                                zx_u32 n_filenames, char const *tgt_file_filter,
                                zx_u64 tgt_file_filter_len, bool *is_tgt) {
  zx_u32 tgt_count = 0;
  for (zx_u32 i = 0; i < n_filenames; i++) {
    char const *filename = filenames[i];
    zx_u64 const filename_len = __builtin_strlen(filename);
    is_tgt[i] = nullptr != zx_memmem((zx_u8 const *)filename, filename_len,
                                     (zx_u8 const *)tgt_file_filter,
                                     tgt_file_filter_len);
    if (is_tgt[i]) {
      tgt_count++;
    }
  }

  return tgt_count;
}

typedef struct ZXFuzzer {
  void *usr_ctx;
  zx_u64 *fuzz_input;
  ZXFuzzValues *fuzz_values;
  bool *marker_coverage;
  bool *err_coverage;
  bool *marker_is_tgt;
  zx_u32 tgt_marker_count;
  bool *err_is_tgt;
  zx_u32 tgt_err_count;
  zx_fuzz_one_fn fuzz_one;
  ZXFuzzLinkerSet linker_set;
  zx_u32 *signature_dedup;
  zx_u32 signature_dedup_cap;
  zx_u32 fuzz_input_len;
} ZXFuzzer;

static ZXStatus ZXFuzzer_init(ZXAlloc *alloc, ZXFuzzParams const *params,
                              zx_u32 fuzz_input_len, zx_u32 signature_dedup_cap,
                              ZXFuzzer *fuzzer, ZXErrStack *err_stack) {
  static_assert(sizeof(bool) == 1);

  ZXFuzzLinkerSet const linker_set = ZXFuzzLinkerSet_load();

  zx_u64 *fuzz_input = nullptr;
  zx_u32 const fuzz_input_byte_size =
      zx_chk_mul_u32(fuzz_input_len, sizeof(zx_u64));
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u64), fuzz_input_byte_size,
                            (zx_u8 **)&fuzz_input, err_stack)),
             err_stack);
  __builtin_memset(fuzz_input, 0, fuzz_input_byte_size);

  ZXFuzzValues *fuzz_values = nullptr;
  zx_u32 const fuzz_values_byte_size =
      zx_chk_mul_u32(fuzz_input_len, sizeof(ZXFuzzValues));
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(ZXFuzzValues), fuzz_values_byte_size,
                            (zx_u8 **)&fuzz_values, err_stack)),
             err_stack);
  __builtin_memset(fuzz_values, 0, fuzz_values_byte_size);

  bool *marker_coverage = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), linker_set.n_markers,
                            (zx_u8 **)&marker_coverage, err_stack)),
             err_stack);
  __builtin_memset(marker_coverage, 0, linker_set.n_markers);

  bool *err_coverage = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), linker_set.n_errs,
                            (zx_u8 **)&err_coverage, err_stack)),
             err_stack);
  __builtin_memset(err_coverage, 0, linker_set.n_errs);

  zx_u32 *signature_dedup = nullptr;
  zx_u32 const signature_dedup_byte_size = signature_dedup_cap * sizeof(zx_u32);
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u32), signature_dedup_byte_size,
                            (zx_u8 **)&signature_dedup, err_stack)),
             err_stack);
  __builtin_memset(signature_dedup, 0, signature_dedup_byte_size);

  bool *marker_is_tgt = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), linker_set.n_markers,
                            (zx_u8 **)&marker_is_tgt, err_stack)),
             err_stack);
  zx_u32 const tgt_marker_count = zx_fuzz_count_tgt(
      linker_set.marker_filename, linker_set.n_markers, params->tgt_file_filter,
      params->tgt_file_filter_len, marker_is_tgt);

  bool *err_is_tgt = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), linker_set.n_errs,
                            (zx_u8 **)&err_is_tgt, err_stack)),
             err_stack);
  zx_u32 const tgt_err_count = zx_fuzz_count_tgt(
      linker_set.err_filename, linker_set.n_errs, params->tgt_file_filter,
      params->tgt_file_filter_len, err_is_tgt);

  *fuzzer = (ZXFuzzer){
      .usr_ctx = params->usr_ctx,
      .fuzz_input = fuzz_input,
      .fuzz_values = fuzz_values,
      .marker_coverage = marker_coverage,
      .err_coverage = err_coverage,
      .marker_is_tgt = marker_is_tgt,
      .err_is_tgt = err_is_tgt,
      .fuzz_one = params->fuzz_one,
      .linker_set = linker_set,
      .signature_dedup = signature_dedup,
      .signature_dedup_cap = signature_dedup_cap,
      .fuzz_input_len = fuzz_input_len,
      .tgt_err_count = tgt_err_count,
      .tgt_marker_count = tgt_marker_count,
  };

  return ZXStatus_Ok;
}

static ZXStatus ZXFuzzer_print_status(ZXFuzzer *fuzzer, bool clear_lines,
                                      ZXFmt *fmt, ZXErrStack *err_stack) {
  zx_u32 const marker_cov = zx_fuzz_count_coverage(
      fuzzer->marker_coverage, fuzzer->linker_set.n_markers);
  zx_u32 const err_cov =
      zx_fuzz_count_coverage(fuzzer->err_coverage, fuzzer->linker_set.n_errs);
  zx_u32 const tgt_marker_cov =
      zx_fuzz_count_tgt_coverage(fuzzer->marker_is_tgt, fuzzer->marker_coverage,
                                 fuzzer->linker_set.n_markers);
  zx_u32 const tgt_err_cov = zx_fuzz_count_tgt_coverage(
      fuzzer->err_is_tgt, fuzzer->err_coverage, fuzzer->linker_set.n_errs);

  if (clear_lines) {
    ZX_FMT(fmt, "\033[2J\033[H");
  }

  ZX_FMT(fmt, "FUZZ STATUS: \n\tMARKERS: ", marker_cov, " / ",
         fuzzer->linker_set.n_markers, (char)'\n');
  ZX_FMT(fmt, "\tERRORS: ", err_cov, " / ", fuzzer->linker_set.n_errs,
         (char)'\n');
  ZX_FMT(fmt, "\tABORTS: ", fuzzer->linker_set.n_aborts, (char)'\n');
  ZX_FMT(fmt, "\tTARGET_MARKERS: ", tgt_marker_cov, " / ",
         fuzzer->tgt_marker_count, (char)'\n');
  ZX_FMT(fmt, "\tTARGET_ERRORS: ", tgt_err_cov, " / ", fuzzer->tgt_err_count,
         (char)'\n');

  ZX_FMT(fmt, "\tUNCOVERED_MARKERS:\n\t\t");

  for (zx_u32 i = 0; i < fuzzer->linker_set.n_markers; i++) {
    if (fuzzer->marker_is_tgt[i] && !fuzzer->marker_coverage[i]) {
      ZX_FMT(fmt, (char)' ', fuzzer->linker_set.marker_filename[i],
             "[line=", fuzzer->linker_set.marker_lineno[i], (char)']');
    }
  }

  ZX_FMT(fmt, "\n\tUNCOVERED_ERRS:\n\t\t");

  for (zx_u32 i = 0; i < fuzzer->linker_set.n_errs; i++) {
    if (fuzzer->err_is_tgt[i] && !fuzzer->err_coverage[i]) {
      ZX_FMT(fmt, (char)' ', fuzzer->linker_set.err_filename[i],
             "[line=", fuzzer->linker_set.err_lineno[i], (char)']');
    }
  }

  ZX_FMT(fmt, (char)'\n');

  ZX_ERR_TRY((zx_io_print(ZX_STDERR, fmt, err_stack)), err_stack);

  ZXFmt_reset(fmt);

  return ZXStatus_Ok;
}

typedef struct ZXFuzzRunResult {
  zx_u64 score;
  zx_u32 consumed_count;
  bool improvement;
} ZXFuzzRunResult;

static ZXStatus ZXFuzzer_fuzz_one(ZXFuzzer *fuzzer, zx_u32 mut_idx,
                                  zx_u64 base_score, ZXFuzzRunResult *result,
                                  ZXErrStack *err_stack) {
  ZX_ASSERT((mut_idx < fuzzer->fuzz_input_len));

  ZXFuzzInput input =
      ZXFuzzInput_init(fuzzer->fuzz_input, fuzzer->fuzz_input_len);

  zx_fuzz_clear_linker_set(&fuzzer->linker_set);

  ZX_ERR_TRY((fuzzer->fuzz_one(fuzzer->usr_ctx, &input, err_stack)), err_stack);

  zx_u64 const signature = zx_fuzz_calculate_signature(&fuzzer->linker_set);
  zx_u64 const score = zx_fuzz_calculate_score(&fuzzer->linker_set);

  bool const sig_is_new = zx_fuzz_process_signature_is_new(
      fuzzer->signature_dedup, fuzzer->signature_dedup_cap, signature);

  zx_u32 const new_coverage = zx_fuzz_update_coverage(
      &fuzzer->linker_set, fuzzer->marker_coverage, fuzzer->err_coverage);
  bool const score_improvement = score > base_score;

  bool const improvement = sig_is_new && (new_coverage || score_improvement);

  if (improvement) {
    ZXFuzzValues_push(&fuzzer->fuzz_values[mut_idx],
                      fuzzer->fuzz_input[mut_idx]);
  }

  *result = (ZXFuzzRunResult){
      .score = score,
      .consumed_count = ZXFuzzInput_consumed_count(&input),
      .improvement = improvement,
  };

  return ZXStatus_Ok;
}

static ZXStatus ZXFuzzer_fuzz(ZXFuzzer *fuzzer, ZXFmt *fmt,
                              ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXFuzzer_print_status(fuzzer, false, fmt, err_stack)), err_stack);

  constexpr zx_u64 MAGIC[] = {0, ZX_U64_MAX};
  constexpr zx_u64 N_MAGIC = sizeof(MAGIC) / sizeof(MAGIC[0]);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);

  ZXXoshiro256_fill(&rand, (zx_u8 *)fuzzer->fuzz_input,
                    fuzzer->fuzz_input_len * sizeof(zx_u64));

  zx_u32 mut_idx = 0;
  zx_u32 fuzz_iter_no = 0;
  while (true) {
    ZX_ASSERT((mut_idx < fuzzer->fuzz_input_len));

    // do a baseline run
    ZXFuzzRunResult baseline_result = {0};
    ZX_ERR_TRY((ZXFuzzer_fuzz_one(fuzzer, mut_idx, ZX_U64_MAX, &baseline_result,
                                  err_stack)),
               err_stack);
    // select a random position to mutate
    mut_idx = zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand),
                              baseline_result.consumed_count);

    for (zx_u64 i = 0; i < N_MAGIC; i++) {
      fuzzer->fuzz_input[mut_idx] = MAGIC[i];
      ZXFuzzRunResult result = {0};
      ZX_ERR_TRY((ZXFuzzer_fuzz_one(fuzzer, mut_idx, baseline_result.score,
                                    &result, err_stack)),
                 err_stack);
    }

    // try previous interesting values for this mut_idx
    ZXFuzzValues const *fuzz_vals = &fuzzer->fuzz_values[mut_idx];
    for (zx_u64 i = 0; i < fuzz_vals->n_filled; i++) {
      fuzzer->fuzz_input[mut_idx] = fuzz_vals->values[i];

      ZXFuzzRunResult result = {0};
      ZX_ERR_TRY((ZXFuzzer_fuzz_one(fuzzer, mut_idx, baseline_result.score,
                                    &result, err_stack)),
                 err_stack);
    }

    // try some random values in this position
    // move on if we can't improve coverage for some number of
    // iterations
    // do more iterations for interesting mut_idx
    zx_u32 const max_stall = zx_chk_add_u32(8, fuzz_vals->n_filled);
    zx_u32 stall = 0;
    while (stall < max_stall) {
      zx_u64 const new_val = ZXXoshiro256_next(&rand);
      fuzzer->fuzz_input[mut_idx] = new_val;

      ZXFuzzRunResult result = {0};
      ZX_ERR_TRY((ZXFuzzer_fuzz_one(fuzzer, mut_idx, baseline_result.score,
                                    &result, err_stack)),
                 err_stack);

      if (result.improvement) {
        stall = 0;
      } else {
        stall += 1;
      }
    }

    if (fuzz_vals->n_filled > 0) {
      zx_u32 const val_idx = zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(&rand),
                                             fuzz_vals->n_filled);
      fuzzer->fuzz_input[mut_idx] = fuzz_vals->values[val_idx];
    } else {
      fuzzer->fuzz_input[mut_idx] = ZXXoshiro256_next(&rand);
    }

    if ((fuzz_iter_no % 1024) == 0) {
      ZX_ERR_TRY((ZXFuzzer_print_status(fuzzer, true, fmt, err_stack)),
                 err_stack);
    }

    fuzz_iter_no += 1;
  }

  return ZXStatus_Ok;
}

ZXStatus zx_fuzz_main(ZXAlloc *alloc, ZXFuzzParams const *params,
                      ZXErrStack *err_stack) {
  constexpr zx_u32 FMT_BUF_CAP = 1 << 14;
  constexpr zx_u32 INPUT_LEN = 1 << 14;
  constexpr zx_u32 SIGNATURE_DEDUP_CAP = 1 << 21;

  char *fmt_buf = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(char), FMT_BUF_CAP,
                            (zx_u8 **)&fmt_buf, err_stack)),
             err_stack);
  ZXFmt fmt = ZXFmt_init(fmt_buf, FMT_BUF_CAP);

  ZXFuzzer fuzzer = {0};
  ZX_ERR_TRY((ZXFuzzer_init(alloc, params, INPUT_LEN, SIGNATURE_DEDUP_CAP,
                            &fuzzer, err_stack)),
             err_stack);

  ZX_ERR_TRY((ZXFuzzer_fuzz(&fuzzer, &fmt, err_stack)), err_stack);

  return ZXStatus_Ok;
}

#endif

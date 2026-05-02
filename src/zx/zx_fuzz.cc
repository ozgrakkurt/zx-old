#ifdef ZX_FUZZ

#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slice.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

// CONFIG
constexpr zx_u64 ZX_FUZZ_INPUT_CAP = 1024;
constexpr zx_u64 ZX_FUZZ_SIGNATURE_DEDUP_CAP = 4096;
constexpr zx_u64 ZX_FUZZ_MAX_SECONDS = 30;
constexpr zx_u64 ZX_FUZZ_MAX_N_ITER = zx_u64{1} << 24;
constexpr zx_u64 ZX_FUZZ_MAGIC[] = {0, ZX_U64_MAX};
constexpr zx_u64 ZX_FUZZ_N_MAGIC =
    sizeof(ZX_FUZZ_MAGIC) / sizeof(ZX_FUZZ_MAGIC[0]);
// CONFIG

static zx_u32* ZX_FUZZ_SAN_COV_ = nullptr;
static zx_u64 ZX_FUZZ_SAN_COV_LEN_ = 0;

extern "C" {

void __sanitizer_cov_trace_pc_guard_init(zx_u32_c* start, zx_u32_c* stop) {
  if (start == nullptr) {
    __builtin_trap();
  }
  if (stop == nullptr) {
    __builtin_trap();
  }

  static_assert(sizeof(void*) == sizeof(zx_u64));

  zx_u64 const start_addr = (zx_u64)start;
  zx_u64 const stop_addr = (zx_u64)stop;

  if (stop_addr < start_addr) {
    __builtin_trap();
  }
  zx_u64 const n_bytes = zx_sub_unsafe_u64(stop_addr, start_addr);

  if (n_bytes % sizeof(zx_u32) != 0) {
    __builtin_trap();
  }
  zx_u64 const n_slots = zx_div_unsafe_u64(n_bytes, sizeof(zx_u32));

  if (ZX_FUZZ_SAN_COV_ != nullptr) {
    __builtin_trap();
  }
  if (ZX_FUZZ_SAN_COV_LEN_ != 0) {
    __builtin_trap();
  }
  ZX_FUZZ_SAN_COV_ = (zx_u32*)start;
  ZX_FUZZ_SAN_COV_LEN_ = n_slots;
}

__attribute__((no_builtin, no_sanitize("coverage"))) void
__sanitizer_cov_trace_pc_guard(zx_u32_c* guard_variable) {
  *guard_variable = (*guard_variable + 1__wbu) | zx_u32{1};
}

}  // extern "C"

__attribute__((no_builtin, no_sanitize("coverage"))) static void
zx_fuzz_reset_coverage() {
  zx_u32* cov = ZX_FUZZ_SAN_COV_;
  zx_u64 len = ZX_FUZZ_SAN_COV_LEN_;
  for (zx_u64 i = 0; i < len; i++) {
    cov[i] = 0;
  }
}

__attribute__((no_builtin, no_sanitize("coverage"))) static void
zx_fuzz_load_coverage(zx_u32* dst, zx_u64 dst_len) {
  zx_u32 const* src = ZX_FUZZ_SAN_COV_;
  zx_u64 src_len = ZX_FUZZ_SAN_COV_LEN_;
  if (src_len != dst_len) {
    __builtin_trap();
  }
  for (zx_u64 i = 0; i < dst_len; i++) {
    dst[i] = src[i];
  }
}

static void zx_fuzz_assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

constexpr zx_u32 ZX_FUZZ_SLOT_SET_CAP = 512;
struct ZXFuzzSlotSet {
  zx_u64 values[ZX_FUZZ_SLOT_SET_CAP];
  zx_u32 push_pos;
  zx_u32 n_filled;
};

static void zx_fuzz_slot_set_push(ZXFuzzSlotSet* set, zx_u64 value) {
  zx_fuzz_assert(set != nullptr);
  constexpr zx_u64 val_cap = sizeof(set->values) / sizeof(set->values[0]);
  static_assert(val_cap > 0);
  zx_u32 const idx = zx_mod_unsafe_u32(set->push_pos, val_cap);
  set->values[idx] = value;
  set->push_pos = zx_wrapping_add_u32(set->push_pos, 1);
  set->n_filled = zx_min_u32(zx_wrapping_add_u32(set->n_filled, 1), val_cap);
}

struct ZXFuzzCtx {
  ZXSliceMut<ZXFuzzSlotSet> slot_sets;
  ZXSliceMut<zx_u32> signature_dedup;
  ZXSliceMut<zx_u64> fuzz_input;
  ZXSliceMut<zx_u32> coverage;
  ZXSliceMut<bool> total_coverage;
};

static ZXFuzzCtx zx_fuzz_ctx_alloc(ZXAlloc* alloc) {
  ZXSliceMut<zx_u32> coverage;
  ZXAllocStatus alloc_status =
      alloc->alloc<zx_u32>(ZX_FUZZ_SAN_COV_LEN_, &coverage);
  zx_fuzz_assert(alloc_status == ZXAllocStatus::Ok);
  coverage.memset(0);

  ZXSliceMut<zx_u64> fuzz_input;
  alloc_status = alloc->alloc<zx_u64>(ZX_FUZZ_INPUT_CAP, &fuzz_input);
  zx_fuzz_assert(alloc_status == ZXAllocStatus::Ok);
  fuzz_input.memset(0);

  ZXSliceMut<zx_u32> signature_dedup;
  alloc_status =
      alloc->alloc<zx_u32>(ZX_FUZZ_SIGNATURE_DEDUP_CAP, &signature_dedup);
  zx_fuzz_assert(alloc_status == ZXAllocStatus::Ok);
  signature_dedup.memset(0);

  ZXSliceMut<ZXFuzzSlotSet> slot_sets;
  alloc_status = alloc->alloc<ZXFuzzSlotSet>(ZX_FUZZ_INPUT_CAP, &slot_sets);
  zx_fuzz_assert(alloc_status == ZXAllocStatus::Ok);
  slot_sets.memset(0);

  ZXSliceMut<bool> total_coverage;
  alloc_status = alloc->alloc<bool>(ZX_FUZZ_SAN_COV_LEN_, &total_coverage);
  zx_fuzz_assert(alloc_status == ZXAllocStatus::Ok);
  total_coverage.memset(0);

  return ZXFuzzCtx{
      .slot_sets = slot_sets,
      .signature_dedup = signature_dedup,
      .fuzz_input = fuzz_input,
      .coverage = coverage,
      .total_coverage = total_coverage,
  };
}

static zx_u32 zx_fuzz_calculate_signature(ZXSlice<zx_u32> coverage) {
  return (zx_u32)zx_rapidhash_v3(coverage.cast_unsafe<zx_u8>(), 0);
}

static zx_u64 zx_fuzz_calculate_score(ZXSlice<zx_u32> coverage) {
  zx_u64 score = 0;

  for (zx_u64 i = 0; i < coverage.len(); i++) {
    if (coverage.read_unsafe(i) > 0) {
      score = zx_saturating_add_u64(score, 1);
    }
  }

  return score;
}

static bool zx_fuzz_update_total_coverage(ZXSliceMut<bool> total_coverage,
                                          ZXSlice<zx_u32> coverage) {
  if (total_coverage.len() != coverage.len()) {
    __builtin_trap();
  }

  bool new_coverage = false;
  for (zx_u64 i = 0; i < total_coverage.len(); i++) {
    if (coverage.read_unsafe(i) > 0) {
      total_coverage.write_unsafe(i, true);
      new_coverage = true;
    }
  }

  return new_coverage;
}

struct ZXFuzzResult {
  zx_u64 score;
  zx_u64 consumed_count;
  bool improvement;
};

static bool zx_fuzz_push_signature(ZXSliceMut<zx_u32> signature_dedup,
                                   zx_u32 signature) {
  if (signature_dedup.len() == 0) {
    __builtin_trap();
  }

  zx_u64 const idx = zx_rand_mod_u64(signature, signature_dedup.len());
  zx_u32 const old_sig = signature_dedup.read_unsafe(idx);
  bool const is_new = old_sig != signature;
  signature_dedup.write_unsafe(idx, signature);
  return is_new;
}

static ZXFuzzResult zx_fuzz_wrapper(ZXFuzzCtx* ctx,
                                    zx_u64 base_score,
                                    zx_u64 mut_idx,
                                    void* usr_ctx,
                                    zx_fuzz_one_fn fuzz_one) {
  ZXFuzzInput fuzz_input = ZXFuzzInput::init_(ctx->fuzz_input.as_const());
  zx_u32* coverage = ctx->coverage.ptr();
  zx_u64 coverage_len = ctx->coverage.len();
  // don't break these up
  zx_fuzz_reset_coverage();
  fuzz_one(usr_ctx, &fuzz_input);
  zx_fuzz_load_coverage(coverage, coverage_len);
  // don't break these up

  zx_u64 const consumed_count = fuzz_input.consumed_count_();

  ZXSlice<zx_u32> const cov = ctx->coverage.as_const();
  bool const new_coverage =
      zx_fuzz_update_total_coverage(ctx->total_coverage, cov);
  zx_u32 const signature = zx_fuzz_calculate_signature(cov);
  zx_u64 const score = zx_fuzz_calculate_score(cov);
  bool const sig_is_new =
      zx_fuzz_push_signature(ctx->signature_dedup, signature);

  bool const score_improvement = score > base_score;
  bool const improvement = sig_is_new && (new_coverage || score_improvement);

  if (improvement) {
    zx_fuzz_slot_set_push(ctx->slot_sets.offset_unsafe(mut_idx),
                          ctx->fuzz_input.read_unsafe(mut_idx));
  }

  return ZXFuzzResult{
      .score = score,
      .improvement = improvement,
      .consumed_count = consumed_count,
  };
}

void zx_fuzz(ZXAlloc* alloc, void* usr_ctx, zx_fuzz_one_fn fuzz_one) {
  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  ZXFuzzCtx ctx = zx_fuzz_ctx_alloc(alloc);
  ctx.signature_dedup.fill(0);
  ctx.fuzz_input.fill(0);
  ctx.slot_sets.memset(0);
  ctx.total_coverage.fill(false);

  zx_u64 mut_idx = 0;

  ZXTimeInfo time_info;
  zx_fuzz_assert(ZXTimeStatus::Ok == ZXTimeInfo::init(&time_info));
  ZXTimer timer = ZXTimer::init(time_info);
  for (zx_u64 iter_no = 0;
       iter_no < ZX_FUZZ_MAX_N_ITER &&
       timer.elapsed() < zx_mul_unsafe_u64(ZX_FUZZ_MAX_SECONDS, 1'000'000'000);
       iter_no++) {
    zx_fuzz_assert(mut_idx < ctx.fuzz_input.len());

    // do a baseline run
    ZXFuzzResult const baseline_result =
        zx_fuzz_wrapper(&ctx, ZX_U64_MAX, mut_idx, usr_ctx, fuzz_one);
    // select a random position to mutate
    mut_idx = zx_rand_mod_u64(rand.next(), baseline_result.consumed_count);

    // try magic values
    for (zx_u64 i = 0; i < ZX_FUZZ_N_MAGIC; i++) {
      ctx.fuzz_input.write_unsafe(mut_idx, ZX_FUZZ_MAGIC[i]);
      zx_fuzz_wrapper(&ctx, baseline_result.score, mut_idx, usr_ctx, fuzz_one);
    }

    // try previous interesting values for this mut_idx
    ZXFuzzSlotSet const* slot_set = ctx.slot_sets.offset_unsafe(mut_idx);
    for (zx_u32 i = 0; i < slot_set->n_filled; i++) {
      ctx.fuzz_input.write_unsafe(mut_idx, slot_set->values[i]);
      zx_fuzz_wrapper(&ctx, baseline_result.score, mut_idx, usr_ctx, fuzz_one);
    }

    // try some random values in this position
    // move on if we can't improve coverage for some number of
    // iterations
    // do more iterations for interesting mut_idx
    zx_u32 const max_stall = zx_saturating_add_u32(slot_set->n_filled, 8);
    zx_u32 stall = 0;
    while (stall < max_stall) {
      zx_u64 const new_val = rand.next();
      ctx.fuzz_input.write_unsafe(mut_idx, new_val);

      ZXFuzzResult const result = zx_fuzz_wrapper(&ctx, baseline_result.score,
                                                  mut_idx, usr_ctx, fuzz_one);
      if (result.improvement) {
        stall = 0;
      } else {
        stall = zx_saturating_add_u32(stall, 1);
      }
    }

    if (slot_set->n_filled > 0) {
      zx_u32 const val_idx =
          zx_rand_mod_u32((zx_u32)rand.next(), slot_set->n_filled);
      ctx.fuzz_input.write_unsafe(mut_idx, slot_set->values[val_idx]);
    } else {
      ctx.fuzz_input.write_unsafe(mut_idx, rand.next());
    }
  }
}

#endif  // #ifdef ZX_FUZZ

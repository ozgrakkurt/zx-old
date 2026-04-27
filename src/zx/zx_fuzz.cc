#ifdef ZX_FUZZ

#include <zx/zx_int.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slice.h>

static ZXSliceMut<zx_u32> ZX_FUZZ_SAN_COV_;

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

  ZX_FUZZ_SAN_COV_ = ZXSliceMut::init_unsafe((zx_u32*)start, n_slots);
}

void __sanitizer_cov_trace_pc_guard(zx_u32_c* guard_variable) {
  *guard_variable = zx_wrapping_add_u32(*guard_variable, 1) | zx_u32{1};
}

}  // extern "C"

static zx_u64 zx_fuzz_calculate_signature() {
  return zx_rapidhash_v3(ZX_FUZZ_SAN_COV_.as_const().cast_unsafe<zx_u8>());
}

static zx_u64 zx_fuzz_calculate_score() {
  zx_u64 score = 0;

  for (zx_u64 i = 0; i < ZX_FUZZ_SAN_COV_.len(); i++) {
    if (ZX_FUZZ_SAN_COV_.read_unsafe(i) > 0) {
      score = zx_add_unsafe_u64(score, 1);
    }
  }

  return score;
}

#endif

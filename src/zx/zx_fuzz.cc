#ifdef ZX_FUZZ

#include <zx/zx_int.h>

extern "C" {

void __sanitizer_cov_trace_pc_guard_init(zx_u32_c* start, zx_u32_c* stop) {}

void __sanitizer_cov_trace_pc_guard(zx_u32_c* guard_variable) {}

}  // extern "C"

#endif

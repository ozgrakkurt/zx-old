#ifndef ZX_FUZZ_LINKER_SET_H
#define ZX_FUZZ_LINKER_SET_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#include <zx/zx_def.h>
#include <zx/zx_int.h>

extern zx_u8 __start_zx_fuzz_marker_hit;
extern int const __start_zx_fuzz_marker_lineno;

extern char const *__start_zx_fuzz_marker_filename;
extern char const *__stop_zx_fuzz_marker_filename;

extern zx_u8 __start_zx_fuzz_err_hit;
extern int const __start_zx_fuzz_err_lineno;

extern char const *__start_zx_fuzz_err_filename;
extern char const *__stop_zx_fuzz_err_filename;

extern int const __start_zx_fuzz_abort_lineno;
extern char const *__start_zx_fuzz_abort_filename;
extern char const *__stop_zx_fuzz_abort_filename;

typedef struct ZXFuzzLinkerSet {
  zx_u32 n_markers;
  zx_u8 *marker_hit;
  char const *const *marker_filename;
  int const *marker_lineno;
  zx_u32 n_errs;
  zx_u8 *err_hit;
  char const *const *err_filename;
  int const *err_lineno;
  zx_u32 n_aborts;
  char const *const *abort_filename;
  int const *abort_lineno;
} ZXFuzzLinkerSet;

static ZXFuzzLinkerSet ZXFuzzLinkerSet_load(void) {
  zx_u64 const n_markers = (((zx_u64)&__stop_zx_fuzz_marker_filename) -
                            ((zx_u64)&__start_zx_fuzz_marker_filename)) /
                           sizeof(__start_zx_fuzz_marker_filename);
  zx_u64 const n_errs = (((zx_u64)&__stop_zx_fuzz_err_filename) -
                         ((zx_u64)&__start_zx_fuzz_err_filename)) /
                        sizeof(__start_zx_fuzz_err_filename);
  zx_u64 const n_aborts = (((zx_u64)&__stop_zx_fuzz_abort_filename) -
                           ((zx_u64)&__start_zx_fuzz_abort_filename)) /
                          sizeof(__start_zx_fuzz_abort_filename);

  ZX_ASSERT((n_markers <= ZX_U32_MAX));
  ZX_ASSERT((n_errs <= ZX_U32_MAX));
  ZX_ASSERT((n_aborts <= ZX_U32_MAX));

  return (ZXFuzzLinkerSet){
      .n_markers = (zx_u32)n_markers,
      .marker_hit = &__start_zx_fuzz_marker_hit,
      .marker_filename = &__start_zx_fuzz_marker_filename,
      .marker_lineno = &__start_zx_fuzz_marker_lineno,
      .n_errs = (zx_u32)n_errs,
      .err_hit = &__start_zx_fuzz_err_hit,
      .err_filename = &__start_zx_fuzz_err_filename,
      .err_lineno = &__start_zx_fuzz_err_lineno,
      .n_aborts = (zx_u32)n_aborts,
      .abort_filename = &__start_zx_fuzz_abort_filename,
      .abort_lineno = &__start_zx_fuzz_abort_lineno,
  };
}

#endif

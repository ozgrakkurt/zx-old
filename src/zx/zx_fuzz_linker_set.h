#ifndef ZX_FUZZ_LINKER_SET_H
#define ZX_FUZZ_LINKER_SET_H

#include <zx/zx_def.h>

extern bool __start_zx_fuzz_marker_is_hit;
extern int const __start_zx_fuzz_marker_lineno;

extern char const *__start_zx_fuzz_marker_filename;
extern char const *__stop_zx_fuzz_marker_filename;

extern bool __start_zx_fuzz_err_is_hit;
extern int const __start_zx_fuzz_err_lineno;

extern char const *__start_zx_fuzz_err_filename;
extern char const *__stop_zx_fuzz_err_filename;

extern int const __start_zx_fuzz_abort_lineno;
extern char const *__start_zx_fuzz_abort_filename;
extern char const *__stop_zx_fuzz_abort_filename;

typedef struct ZXFuzzLinkerSet {
  zx_usize n_markers;
  bool *marker_is_hit;
  char const *const *marker_filename;
  int const *marker_lineno;
  zx_usize n_errs;
  bool *err_is_hit;
  char const *const *err_filename;
  int const *err_lineno;
  zx_usize n_aborts;
  char const *const *abort_filename;
  int const *abort_lineno;
} ZXFuzzLinkerSet;

static ZXFuzzLinkerSet ZXFuzzLinkerSet_load(void) {
  zx_usize const n_markers = (((zx_usize)&__stop_zx_fuzz_marker_filename) -
                              ((zx_usize)&__start_zx_fuzz_marker_filename)) /
                             sizeof(void *);
  zx_usize const n_errs = (((zx_usize)&__stop_zx_fuzz_err_filename) -
                           ((zx_usize)&__start_zx_fuzz_err_filename)) /
                          sizeof(void *);
  zx_usize const n_aborts = (((zx_usize)&__stop_zx_fuzz_abort_filename) -
                             ((zx_usize)&__start_zx_fuzz_abort_filename)) /
                            sizeof(void *);

  return (ZXFuzzLinkerSet){
      .n_markers = n_markers,
      .marker_is_hit = &__start_zx_fuzz_marker_is_hit,
      .marker_filename = &__start_zx_fuzz_marker_filename,
      .marker_lineno = &__start_zx_fuzz_marker_lineno,
      .n_errs = n_errs,
      .err_is_hit = &__start_zx_fuzz_err_is_hit,
      .err_filename = &__start_zx_fuzz_err_filename,
      .err_lineno = &__start_zx_fuzz_err_lineno,
      .n_aborts = n_aborts,
      .abort_filename = &__start_zx_fuzz_abort_filename,
      .abort_lineno = &__start_zx_fuzz_abort_lineno,
  };
}

#endif

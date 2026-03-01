#ifndef ZX_FUZZ_INSTR_H
#define ZX_FUZZ_INSTR_H

#ifdef ZX_FUZZING_ENABLED

#define ZX_FUZZ_MARKER                                                         \
  do {                                                                         \
    static bool zx_fuzz_marker_is_hit_                                         \
        __attribute__((section("zx_fuzz_marker_is_hit"), used)) = false;       \
    static char const *zx_fuzz_marker_filename_                                \
        __attribute__((section("zx_fuzz_marker_filename"), used)) = __FILE__;  \
    static int zx_fuzz_marker_lineno_                                          \
        __attribute__((section("zx_fuzz_marker_lineno"), used)) = __LINE__;    \
    (void)zx_fuzz_marker_filename_;                                            \
    (void)zx_fuzz_marker_lineno_;                                              \
    zx_fuzz_marker_is_hit_ = true;                                             \
    (void)zx_fuzz_marker_is_hit_;                                              \
  } while (false)

#define ZX_FUZZ_ERR_MARKER                                                     \
  do {                                                                         \
    static bool zx_fuzz_err_is_hit_                                            \
        __attribute__((section("zx_fuzz_err_is_hit"), used)) = false;          \
    static char const *zx_fuzz_err_filename_                                   \
        __attribute__((section("zx_fuzz_err_filename"), used)) = __FILE__;     \
    static int zx_fuzz_err_lineno_                                             \
        __attribute__((section("zx_fuzz_err_lineno"), used)) = __LINE__;       \
    (void)zx_fuzz_err_filename_;                                               \
    (void)zx_fuzz_err_lineno_;                                                 \
    zx_fuzz_err_is_hit_ = true;                                                \
    (void)zx_fuzz_err_is_hit_;                                                 \
  } while (false)

#define ZX_FUZZ_ABORT_MARKER                                                   \
  do {                                                                         \
    static char const *zx_fuzz_abort_filename_                                 \
        __attribute__((section("zx_fuzz_abort_filename"), used)) = __FILE__;   \
    static int zx_fuzz_abort_lineno_                                           \
        __attribute__((section("zx_fuzz_abort_lineno"), used)) = __LINE__;     \
    (void)zx_fuzz_abort_filename_;                                             \
    (void)zx_fuzz_abort_lineno_;                                               \
  } while (false)

#else // ifdef ZX_FUZZING_ENABLED

#define ZX_FUZZ_MARKER ((void)0)
#define ZX_FUZZ_ABORT_MARKER ((void)0)
#define ZX_FUZZ_ERR_MARKER ((void)0)

#endif // ifdef ZX_FUZZING_ENABLED

#endif

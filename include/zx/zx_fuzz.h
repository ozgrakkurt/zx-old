#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

[[noreturn]]
void zx_fuzz_handle_crash(int signum);

#define ZX_FUZZ_MARKER                                                         \
  do {                                                                         \
    static bool zx_fuzz_marker_is_hit_                                         \
        __attribute__((section("zx_fuzz_marker_is_hit"), used)) = false;       \
    static char const *zx_fuzz_marker_filename_                                \
        __attribute__((section("zx_fuzz_marker_filename"), used)) = __FILE__;  \
    static int zx_fuzz_marker_lineno_                                          \
        __attribute__((section("zx_fuzz_marker_lineno"), used)) = __LINE__;    \
    (void)zx_fuzz_marker_filename_;                                            \
    (void)zx_fuzz_marker_lineno;                                               \
    zx_fuzz_marker_is_hit_ = true;                                             \
  } while (false)

#define ZX_FUZZ_ERR                                                            \
  do {                                                                         \
    static bool zx_fuzz_err_is_hit_                                            \
        __attribute__((section("zx_fuzz_err_is_hit"), used)) = false;          \
    static char const *zx_fuzz_err_filename_                                   \
        __attribute__((section("zx_fuzz_err_filename"), used)) = __FILE__;     \
    static int zx_fuzz_err_lineno_                                             \
        __attribute__((section("zx_fuzz_err_lineno"), used)) = __LINE__;       \
    (void)zx_fuzz_err_filename_;                                               \
    (void)zx_fuzz_err_lineno;                                                  \
    zx_fuzz_err_is_hit_ = true;                                                \
  } while (false)

#define ZX_FUZZ_ABORT                                                          \
  do {                                                                         \
    static char const *zx_fuzz_abort_filename_                                 \
        __attribute__((section("zx_fuzz_abort_filename"), used)) = __FILE__;   \
    static int zx_fuzz_abort_lineno_                                           \
        __attribute__((section("zx_fuzz_abort_lineno"), used)) = __LINE__;     \
    (void)zx_fuzz_abort_filename_;                                             \
    (void)zx_fuzz_abort_lineno;                                                \
  } while (false)

#else // ifdef ZX_FUZZING_ENABLED

#define ZX_FUZZ_MARKER(expr) ((void)0)
#define ZX_FUZZ_ABORT(expr) ((void)0)
#define ZX_FUZZ_ERR(expr) ((void)0)

#endif // ifdef ZX_FUZZING_ENABLED

#endif

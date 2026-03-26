#ifndef ZX_FUZZ_INSTR_H
#define ZX_FUZZ_INSTR_H

#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_def.h>

#define ZX_FUZZ_MARKER                                                         \
  do {                                                                         \
    static zx_u8 zx_fuzz_marker_hit_                                           \
        __attribute__((section("zx_fuzz_marker_hit"), used, retain)) = 0;      \
    static char const *zx_fuzz_marker_filename_                                \
        __attribute__((section("zx_fuzz_marker_filename"), used, retain)) =    \
            __FILE_NAME__;                                                     \
    static zx_u16 zx_fuzz_marker_lineno_                                       \
        __attribute__((section("zx_fuzz_marker_lineno"), used, retain)) =      \
            __LINE__;                                                          \
    (void)zx_fuzz_marker_filename_;                                            \
    (void)zx_fuzz_marker_lineno_;                                              \
    zx_fuzz_marker_hit_ = (zx_fuzz_marker_hit_ + 1wbu) | (zx_u8){1wbu};        \
    (void)zx_fuzz_marker_hit_;                                                 \
  } while (false)

#define ZX_FUZZ_ERR_MARKER                                                     \
  do {                                                                         \
    static zx_u8 zx_fuzz_err_hit_                                              \
        __attribute__((section("zx_fuzz_err_hit"), used, retain)) = 0;         \
    static char const *zx_fuzz_err_filename_                                   \
        __attribute__((section("zx_fuzz_err_filename"), used, retain)) =       \
            __FILE_NAME__;                                                     \
    static zx_u16 zx_fuzz_err_lineno_                                          \
        __attribute__((section("zx_fuzz_err_lineno"), used, retain)) =         \
            __LINE__;                                                          \
    (void)zx_fuzz_err_filename_;                                               \
    (void)zx_fuzz_err_lineno_;                                                 \
    zx_fuzz_err_hit_ = (zx_fuzz_err_hit_ + 1wbu) | (zx_u8){1wbu};              \
    (void)zx_fuzz_err_hit_;                                                    \
  } while (false)

#define ZX_FUZZ_ABORT_MARKER                                                   \
  do {                                                                         \
    static char const *zx_fuzz_abort_filename_                                 \
        __attribute__((section("zx_fuzz_abort_filename"), used, retain)) =     \
            __FILE_NAME__;                                                     \
    static zx_u16 zx_fuzz_abort_lineno_                                        \
        __attribute__((section("zx_fuzz_abort_lineno"), used, retain)) =       \
            __LINE__;                                                          \
    (void)zx_fuzz_abort_filename_;                                             \
    (void)zx_fuzz_abort_lineno_;                                               \
  } while (false)

#else // ifdef ZX_FUZZING_ENABLED

#define ZX_FUZZ_MARKER ((void)0)
#define ZX_FUZZ_ABORT_MARKER ((void)0)
#define ZX_FUZZ_ERR_MARKER ((void)0)

#endif // ifdef ZX_FUZZING_ENABLED

#endif

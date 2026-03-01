#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

#include <zx/zx_fuzz.h>
#include <zx/zx_sys.h>

#define ZX_ASSERT_STRINGIFY_(x) #x
#define ZX_ASSERT_TO_STRING_(x) ZX_ASSERT_STRINGIFY_(x)

#define ZX_ASSERT_INFO_(expr)                                                  \
  "\tFILE=" __FILE__ "; LINE=" ZX_ASSERT_TO_STRING_(                           \
      __LINE__) "; EXPR=(" ZX_ASSERT_TO_STRING_(expr) ");\n"

#define ZX_ASSERT(expr)                                                        \
  do {                                                                         \
    char const *zx_assert_info_ =                                              \
        "Failed assertion at:\n" ZX_ASSERT_INFO_(ZX_ASSERT(expr)) "\n";        \
    bool const zx_assert_success_ = (expr);                                    \
    if (!zx_assert_success_) {                                                 \
      ZX_FUZZ_ABORT;                                                           \
      zx_assert_(zx_assert_info_, __builtin_strlen(zx_assert_info_));          \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

static inline void zx_assert_(char const *info, zx_usize info_len) {
  zx_usize n_written = 0;
  zx_usize const status = zx_sys_write(ZX_STDERR, info, info_len, &n_written);
  (void)status;
  zx_sys_abort();
}

#ifdef ZX_DBG_ASSERT_ENABLED
#define ZX_DBG_ASSERT(expr) ZX_ASSERT(expr)
#else
#define ZX_DBG_ASSERT(expr) ((void)0)
#endif

#endif

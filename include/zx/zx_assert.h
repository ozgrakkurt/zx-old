#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

#include <zx/zx_sys.h>

#define ZX_ASSERT_COLOR_RED_ "\x1B[31m"
#define ZX_ASSERT_COLOR_RESET_ "\x1B[0m"

#define ZX_ASSERT_STRINGIFY_(x) #x
#define ZX_ASSERT_TO_STRING_(x) ZX_ASSERT_STRINGIFY_(x)

#define ZX_ASSERT_INFO_(expr)                                                  \
  "\tFILE=" __FILE__ "; LINE=" ZX_ASSERT_TO_STRING_(                           \
      __LINE__) "; EXPR=(" ZX_ASSERT_TO_STRING_(expr) ");\n"

#define ZX_ASSERT(expr)                                                        \
  do {                                                                         \
    bool const v = (expr);                                                     \
    if (!v) {                                                                  \
      char const *err_str = ZX_ASSERT_COLOR_RED_                               \
          "Failed assertion at:\n" ZX_ASSERT_INFO_(ZX_ASSERT(expr))            \
              ZX_ASSERT_COLOR_RESET_ "\n";                                     \
      zx_usize n_written = 0;                                                  \
      (void)zx_sys_write(ZX_STDERR, err_str, __builtin_strlen(err_str),        \
                         &n_written);                                          \
      zx_sys_abort();                                                          \
    }                                                                          \
  } while (false)

#ifdef ZX_DBG_ASSERT_ENABLED
#define ZX_DBG_ASSERT(expr) ZX_ASSERT(expr)
#else
#define ZX_DBG_ASSERT(expr) ((void)0)
#endif

#endif

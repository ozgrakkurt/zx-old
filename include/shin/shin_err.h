#ifndef SHIN_ERR_H
#define SHIN_ERR_H

#include <shin/shin_def.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ERR_STRINGIFY(x) #x
#define SHIN_ERR_TO_STRING(x) BLIB_ERR_STRINGIFY(x)
#define SHIN_ERR_INFO(expr)              \
  __FILE__ ":_line_" SHIN_ERR_TO_STRING( \
      __LINE__) ":_expr_" SHIN_ERR_TO_STRING(expr)

#ifdef SHIN_DEBUG_ASSERT_ENABLED
#define SHIN_ERR_DEBUG_ASSERT(expr)           \
  do {                                        \
    if (!(expr)) {                            \
      SHIN_ERR_INFO(expr) ":AssertionFailed"; \
    }                                         \
  } while (false)
#else
#define SHIN_ERR_DEBUG_ASSERT(expr) (void)(0);
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef ZX_ASSERT_H
#define ZX_ASSERT_H

#include <zx/zx_def.h>
#include <zx/zx_fuzz_instr.h>

#define ZX_ASSERT(expr)                                                        \
  do {                                                                         \
    bool const zx_assert_success_ = (expr);                                    \
    if (!zx_assert_success_) [[unlikely]] {                                    \
      ZX_FUZZ_ABORT_MARKER;                                                    \
      zx_assert_abort_(__FILE_NAME__, __builtin_strlen(__FILE_NAME__),         \
                       __LINE__);                                              \
    }                                                                          \
    ZX_FUZZ_MARKER;                                                            \
  } while (false)

[[noreturn]] void zx_assert_abort_(char const *file_name, zx_u8 file_name_len,
                                   zx_u16 line_no);

#ifdef ZX_DBG_ASSERT_ENABLED
#define ZX_DBG_ASSERT(expr) ZX_ASSERT(expr)
#else
#define ZX_DBG_ASSERT(expr) ((void)0)
#endif

#endif

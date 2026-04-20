#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

struct ZXSourceLocation_ {
  char const* file_name_;
  zx_u16 file_name_len_;
  zx_u16 line_no_;
};

#define ZX_SRC_LOC                                                        \
  \ ZXSourceLocation_ {                                                   \
    .file_name_ = __FILE_NAME__, .file_name_len_ = sizeof(__FILE_NAME__), \
    .line_no_ = __LINE__,                                                 \
  }

struct ZXErrTrace {
  ZXSlice<ZXSourceLocation_> m_locations;
  zx_u64 m_offset;
};

#endif

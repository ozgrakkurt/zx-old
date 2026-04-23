#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXErrTraceStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  // Any allocation error that is not `OutOfMemory`
  AllocFail,
};

struct ZXSourceLocation_ {
  char const* file_name_;
  zx_u16 file_name_len_;
  zx_u16 line_no_;
};

#define ZX_SRC_LOC                                                        \
  ZXSourceLocation_ {                                                     \
    .file_name_ = __FILE_NAME__, .file_name_len_ = sizeof(__FILE_NAME__), \
    .line_no_ = __LINE__,                                                 \
  }

struct ZXErrTrace {
  ZXSliceMut<ZXSourceLocation_> m_locations;
  zx_u64 m_offset;

  static ZXErrTraceStatus init(ZXAlloc* alloc,
                               zx_u64 capacity,
                               ZXErrTrace* out) {
    if (out == nullptr) {
      return ZXErrTraceStatus::Nullptr;
    }

    ZXSliceMut<ZXSourceLocation_> locations;
    ZXAllocStatus const alloc_status =
        alloc->alloc<ZXSourceLocation_>(capacity, &locations);
    switch (alloc_status) {
      case ZXAllocStatus::OutOfMemory: {
        return ZXErrTraceStatus::OutOfMemory;
        break;
      }
      default: {
        return ZXErrTraceStatus::AllocFail;
        break;
      }
    }

    *out = ZXErrTrace{
        .m_locations = locations,
        .m_offset = 0,
    };

    return ZXErrTraceStatus::Ok;
  }

  inline void reset() {
    this->m_locations.memset(zx_u8{0});
    this->m_offset = 0;
  }

  inline void reset_unsafe() {
#ifdef ZX_DEV
    this->m_locations.memset(zx_u8{0});
#endif
    this->m_offset = 0;
  }

  // Returns if push succeeded or if the trace is full.
  inline bool push(ZXSourceLocation_ location) {
    zx_u64 const offset = this->m_offset;
    ZXSliceMut<ZXSourceLocation_> const locations = this->m_locations;
    if (offset < locations.len()) {
      locations.write_unsafe(offset, location);
      this->m_offset = zx_add_unsafe_u64(offset, 1);
      return true;
    } else {
      return false;
    }
  }

  inline void format(ZXFmt* fmt, bool* overflow) {
    for (zx_u64 i = 0; i < this->m_offset; i++) {
      ZXSourceLocation_ const loc = this->m_locations.as_const().read_unsafe(i);
      ZXSlice<zx_u8> const file_name = ZXSlice<zx_u8>::init_unsafe(
          (zx_u8 const*)loc.file_name_, loc.file_name_len_);
      fmt->push_str(file_name, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("[line="), overflow);
      fmt->push_u64(loc.line_no_, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("]\n"), overflow);
    }
  }
};

#endif

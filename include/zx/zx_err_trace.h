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

static inline ZXCString zx_err_trace_status_to_cstring(
    ZXErrTraceStatus status) {
  switch (status) {
    case ZXErrTraceStatus::Ok: {
      return ZXCString::init_unsafe("ZXErrTraceStatus::Ok");
    }
    case ZXErrTraceStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXErrTraceStatus::Nullptr");
    }
    case ZXErrTraceStatus::OutOfMemory: {
      return ZXCString::init_unsafe("ZXErrTraceStatus::OutOfMemory");
    }
    case ZXErrTraceStatus::AllocFail: {
      return ZXCString::init_unsafe("ZXErrTraceStatus::AllocFail");
    }
    default: {
      return ZXCString::init_unsafe("ZXErrTraceStatus::Unknown");
    }
  }
}

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

struct ZXErrTraceEntry_ {
  char const* file_name_;
  char const* info_;
  zx_u16 file_name_len_;
  zx_u16 info_len_;
  zx_u16 line_no_;
};

struct ZXErrTrace {
  ZXSliceMut<ZXErrTraceEntry_> m_entries;
  zx_u64 m_offset;

  static ZXErrTraceStatus init(ZXAlloc* alloc,
                               zx_u64 capacity,
                               ZXErrTrace* out) {
    if (out == nullptr) {
      return ZXErrTraceStatus::Nullptr;
    }

    ZXSliceMut<ZXErrTraceEntry_> entries;
    ZXAllocStatus const alloc_status =
        alloc->alloc<ZXErrTraceEntry_>(capacity, &entries);
    switch (alloc_status) {
      case ZXAllocStatus::Ok: {
        break;
      }
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
        .m_entries = entries,
        .m_offset = 0,
    };

    return ZXErrTraceStatus::Ok;
  }

  inline void reset() {
    this->m_entries.memset(zx_u8{0});
    this->m_offset = 0;
  }

  inline void reset_unsafe() {
#ifdef ZX_DEV
    this->m_entries.memset(zx_u8{0});
#endif
    this->m_offset = 0;
  }

  // Returns true if push succeeded. Returns false if the trace is full.
  inline bool push(ZXSourceLocation_ location, ZXCString info) {
    zx_u64 const offset = this->m_offset;
    ZXSliceMut<ZXErrTraceEntry_> const entries = this->m_entries;
    ZXErrTraceEntry_ const entry = ZXErrTraceEntry_{
        .file_name_ = location.file_name_,
        .file_name_len_ = location.file_name_len_,
        .line_no_ = location.line_no_,
        .info_ = (char const*)info.slice().ptr(),
        .info_len_ = (zx_u16)info.slice().len(),
    };
    if (offset < entries.len()) {
      entries.write_unsafe(offset, entry);
      this->m_offset = zx_add_unsafe_u64(offset, 1);
      return true;
    } else {
      return false;
    }
  }

  inline void format(ZXFmt* fmt, bool* overflow) {
    for (zx_u64 i = 0; i < this->m_offset; i++) {
      ZXErrTraceEntry_ const entry = this->m_entries.read_unsafe(i);
      ZXSlice<zx_u8> const file_name = ZXSlice<zx_u8>::init_unsafe(
          (zx_u8 const*)entry.file_name_, entry.file_name_len_);
      ZXSlice<zx_u8> const info = ZXSlice<zx_u8>::init_unsafe(
          (zx_u8 const*)entry.info_, entry.info_len_);
      fmt->push_str(file_name, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("[line="), overflow);
      fmt->push_u64(entry.line_no_, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("]. Info = "), overflow);
      fmt->push_str(info, overflow);
      fmt->push_char('\n', overflow);
    }
  }
};

#endif

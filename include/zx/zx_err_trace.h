#ifndef ZX_ERR_TRACE_H
#define ZX_ERR_TRACE_H

#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXErrTraceInitStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  // Any allocation error that is not `OutOfMemory`
  AllocFail,
};

struct ZXSourceLocation_ {
  char const *file_name_;
  zx_u16 file_name_len_;
  zx_u16 line_no_;
};

#define ZX_SRC_LOC                                                             \
  ZXSourceLocation_ {                                                          \
    .file_name_ = __FILE_NAME__,                                               \
    .file_name_len_ = __builtin_strlen(__FILE_NAME__), .line_no_ = __LINE__,   \
  }

struct ZXErrTraceEntry_ {
  char const *file_name_;
  zx_u16 file_name_len_;
  zx_u16 line_no_;
  zx_u8 status_code_;
};

struct ZXErrTrace {
  ZXSliceMut<ZXErrTraceEntry_> m_entries;
  zx_u64 m_offset;

  static ZXErrTraceInitStatus init(ZXAlloc *alloc, zx_u64 capacity,
                                   ZXErrTrace *out) {
    if (out == nullptr) {
      return ZXErrTraceInitStatus::Nullptr;
    }

    ZXSliceMut<ZXErrTraceEntry_> entries;
    ZXAllocStatus const alloc_status =
        alloc->alloc<ZXErrTraceEntry_>(capacity, &entries);
    switch (alloc_status) {
    case ZXAllocStatus::Ok: {
      break;
    }
    case ZXAllocStatus::OutOfMemory: {
      return ZXErrTraceInitStatus::OutOfMemory;
      break;
    }
    default: {
      return ZXErrTraceInitStatus::AllocFail;
      break;
    }
    }

    *out = ZXErrTrace{
        .m_entries = entries,
        .m_offset = 0,
    };

    return ZXErrTraceInitStatus::Ok;
  }

  inline void reset() {
#ifdef ZX_DEV
    this->m_entries.memset(zx_u8{0});
#endif
    this->m_offset = 0;
  }

  // Returns true if push succeeded. Returns false if the trace is full.
  inline bool push_(ZXSourceLocation_ location, zx_u8 status_code) {
    zx_u64 const offset = this->m_offset;
    ZXSliceMut<ZXErrTraceEntry_> const entries = this->m_entries;
    ZXErrTraceEntry_ const entry = ZXErrTraceEntry_{
        .file_name_ = location.file_name_,
        .file_name_len_ = location.file_name_len_,
        .line_no_ = location.line_no_,
        .status_code_ = status_code,
    };
    if (offset < entries.len()) {
      entries.write_unsafe(offset, entry);
      this->m_offset = zx_add_unsafe_u64(offset, 1);
      return true;
    } else {
      return false;
    }
  }

  inline void format(ZXFmt *fmt, bool *overflow) {
    for (zx_u64 i = 0; i < this->m_offset; i++) {
      ZXErrTraceEntry_ const entry = this->m_entries.read_unsafe(i);
      ZXSlice<zx_u8> const file_name = ZXSlice<zx_u8>::init_unsafe(
          (zx_u8 const *)entry.file_name_, entry.file_name_len_);
      fmt->push_str(file_name, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("[line="), overflow);
      fmt->push_u64(entry.line_no_, overflow);
      fmt->push_cstring(ZXCString::init_unsafe("]. StatusCode = "), overflow);
      fmt->push_u64(entry.status_code_, overflow);
      fmt->push_char('\n', overflow);
    }
  }
};

static inline bool zx_err_trace_push(ZXErrTrace *err_trace,
                                     ZXSourceLocation_ location,
                                     zx_u8 status_code) {
  if (err_trace != nullptr) {
    return err_trace->push_(location, status_code);
  } else {
    return false;
  }
}

#endif

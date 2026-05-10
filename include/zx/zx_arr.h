#ifndef ZX_ARR_H
#define ZX_ARR_H

#include <zx/zx_err_trace.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXArrInitStatus : zx_u8_c {
  Ok = 0,
  OutOfMemory,
  AllocFail,
  Nullptr,
};

enum class [[nodiscard]] ZXArrPushStatus : zx_u8_c {
  Ok = 0,
  OutOfCapacity,
  Nullptr,
};

template <typename T> struct ZXArr {
  ZXSliceMut<T> m_entries;
  zx_u64 m_offset;

  static inline ZXArrInitStatus init(ZXErrTrace *trace, ZXAlloc *alloc,
                                     zx_u64 capacity, ZXArr<T> *out) {
    if (alloc == nullptr || out == nullptr) {
      return ZXArrInitStatus::Nullptr;
    }

    ZXSliceMut<T> entries;
    switch (ZXAllocStatus const status = alloc->alloc<T>(capacity, &entries);
            status) {
    case ZXAllocStatus::Ok: {
      break;
    }
    case ZXAllocStatus::OutOfMemory: {
      zx_err_trace_push(trace, ZX_SRC_LOC, (zx_u8)status);
      return ZXArrInitStatus::OutOfMemory;
      break;
    }
    default: {
      zx_err_trace_push(trace, ZX_SRC_LOC, (zx_u8)status);
      return ZXArrInitStatus::AllocFail;
      break;
    }
    }

    *out = ZXArr{
        .m_entries = entries,
        .m_offset = 0,
    };
    return ZXArrInitStatus::Ok;
  }

  inline zx_u64 capacity() const { return this->m_entries.len(); }

  inline zx_u64 len() const { return this->m_offset; }

  inline zx_u64 capacity_left() const {
    return zx_sub_unsafe_u64(this->m_entries.len(), this->m_offset);
  }

  inline ZXSlice<T> slice() const {
    return this->m_entries.as_const().slice_unsafe(0, this->m_offset);
  }

  inline ZXSliceMut<T> slice_mut() {
    return this->m_entries.slice_unsafe(0, this->m_offset);
  }

  inline void push_unsafe(T const *entry) {
    zx_assert_dev(entry != nullptr);
    zx_assert_dev(this->capacity_left() > 0);
    zx_u64 const offset = this->m_offset;
    this->m_entries.write_unsafe(offset, *entry);
    this->m_offset = zx_add_unsafe_u64(offset, 1);
  }

  inline ZXArrPushStatus push(T const *entry) {
    if (entry == nullptr) {
      return ZXArrPushStatus::Nullptr;
    }

    if (this->capacity_left() == 0) {
      return ZXArrPushStatus::OutOfCapacity;
    }

    zx_u64 const offset = this->m_offset;
    this->m_entries.write_unsafe(offset, *entry);
    this->m_offset = zx_add_unsafe_u64(offset, 1);

    return ZXArrPushStatus::Ok;
  }
};

#endif

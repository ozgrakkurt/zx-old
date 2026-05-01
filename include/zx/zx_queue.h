#ifndef ZX_QUEUE_H
#define ZX_QUEUE_H

#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXQueueInitStatus : zx_u8_c {
  Ok = 0,
  OutOfMemory,
  AllocFail,
  IntegerOverflow,
  Nullptr,
};

static inline ZXCString zx_queue_init_status_to_cstring(
    ZXQueueInitStatus status) {
  switch (status) {
    case ZXQueueInitStatus::Ok: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::Ok");
    }
    case ZXQueueInitStatus::OutOfMemory: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::OutOfMemory");
    }
    case ZXQueueInitStatus::AllocFail: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::AllocFail");
    }
    case ZXQueueInitStatus::IntegerOverflow: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::IntegerOverFlow");
    }
    case ZXQueueInitStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::Nullptr");
    }
    default: {
      return ZXCString::init_unsafe("ZXQueueInitStatus::Unknown");
    }
  }
}

enum class [[nodiscard]] ZXQueuePushStatus : zx_u8_c {
  Ok = 0,
  OutOfCapacity,
};

static inline ZXCString zx_queue_push_status_to_cstring(
    ZXQueuePushStatus status) {
  switch (status) {
    case ZXQueuePushStatus::Ok: {
      return ZXCString::init_unsafe("ZXQueuePushStatus::Ok");
    }
    case ZXQueuePushStatus::OutOfCapacity: {
      return ZXCString::init_unsafe("ZXQueuePushStatus::OutOfCapacity");
    }
    default: {
      return ZXCString::init_unsafe("ZXQueuePushStatus::Unknown");
    }
  }
}

enum class [[nodiscard]] ZXQueuePopStatus : zx_u8_c {
  Ok = 0,
  Empty,
  Nullptr,
};

static inline ZXCString zx_queue_pop_status_to_cstring(
    ZXQueuePopStatus status) {
  switch (status) {
    case ZXQueuePopStatus::Ok: {
      return ZXCString::init_unsafe("ZXQueuePopStatus::Ok");
    }
    case ZXQueuePopStatus::Empty: {
      return ZXCString::init_unsafe("ZXQueuePopStatus::Empty");
    }
    default: {
      return ZXCString::init_unsafe("ZXQueuePopStatus::Unknown");
    }
  }
}

template <typename T>
struct ZXQueue {
  ZXSliceMut<T> m_entries;
  zx_u32 m_start;
  zx_u32 m_len;
  zx_u32 m_cap_mask;
  zx_u32 m_capacity;

  static inline ZXQueueInitStatus init(ZXAlloc* alloc,
                                       zx_u32 capacity,
                                       ZXQueue* queue) {
    if (alloc == nullptr || queue == nullptr) {
      return ZXQueueInitStatus::Nullptr;
    }

    bool overflow = false;

    zx_u32 const cap = zx_next_pow_two_u32(capacity, &overflow);

    if (overflow) {
      return ZXQueueInitStatus::IntegerOverflow;
    }

    ZXSliceMut<T> entries;
    switch (alloc->alloc<T>(capacity, &entries)) {
      case ZXAllocStatus::Ok: {
        break;
      }
      case ZXAllocStatus::OutOfMemory: {
        return ZXQueueInitStatus::OutOfMemory;
        break;
      }
      default: {
        return ZXQueueInitStatus::AllocFail;
        break;
      }
    }

    *queue = ZXQueue{
        .m_entries = entries,
        .m_start = 0,
        .m_len = 0,
        .m_cap_mask = zx_sub_unsafe_u32(cap, 1),
        .m_capacity = capacity,
    };

    return ZXQueueInitStatus::Ok;
  }

  inline void reset() {
#ifdef ZX_DEV
    this->m_entries.memset(0xaa);
#endif
    this->m_start = 0;
    this->m_len = 0;
  }

  inline zx_u32 capacity_left() const {
    return zx_sub_unsafe_u32(this->m_capacity, this->m_len);
  }

  inline zx_u32 len() const { return this->m_len; }

  inline ZXQueuePushStatus push(T const* entry) {
    if (this->m_capacity == this->m_len) {
      return ZXQueuePushStatus::OutOfCapacity;
    }

    zx_u32 const end =
        zx_add_unsafe_u32(this->m_start, this->m_len) & this->m_cap_mask;

    this->m_entries.write_unsafe(end, *entry);
    this->m_len = zx_add_unsafe_u32(this->m_len, 1);
    return ZXQueuePushStatus::Ok;
  }

  inline ZXQueuePopStatus pop(T* entry) {
    if (entry == nullptr) {
      return ZXQueuePopStatus::Nullptr;
    }

    if (this->m_len == 0) {
      return ZXQueuePopStatus::Empty;
    }

    zx_u32 const start = this->m_start;
    *entry = this->m_entries.read_unsafe(start);

    this->m_start = zx_add_unsafe_u32(start, 1) & this->m_cap_mask;
    this->m_len = zx_sub_unsafe_u32(this->m_len, 1);

    return ZXQueuePopStatus::Ok;
  }
};

#endif

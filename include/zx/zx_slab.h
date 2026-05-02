#ifndef ZX_SLAB_H
#define ZX_SLAB_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXSlabInitStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  AllocFail,
};

static inline ZXCString zx_slab_init_status_to_cstring(
    ZXSlabInitStatus status) {
  switch (status) {
    case ZXSlabInitStatus::Ok: {
      return ZXCString::init_unsafe("ZXSlabInitStatus::Ok");
    }
    case ZXSlabInitStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXSlabInitStatus::Nullptr");
    }
    case ZXSlabInitStatus::OutOfMemory: {
      return ZXCString::init_unsafe("ZXSlabInitStatus::OutOfMemory");
    }
    case ZXSlabInitStatus::AllocFail: {
      return ZXCString::init_unsafe("ZXSlabInitStatus::AllocFail");
    }
    default: {
      return ZXCString::init_unsafe("ZXSlabInitStatus::Unknown");
    }
  }
}

enum class [[nodiscard]] ZXSlabInsertStatus : zx_u8_c {
  Ok = 0,
  OutOfCapacity,
  Nullptr,
};

static inline ZXCString zx_hashtable_insert_status_to_cstring(
    ZXSlabInsertStatus status) {
  switch (status) {
    case ZXSlabInsertStatus::Ok: {
      return ZXCString::init_unsafe("ZXSlabInsertStatus::Ok");
    }
    case ZXSlabInsertStatus::OutOfCapacity: {
      return ZXCString::init_unsafe("ZXSlabInsertStatus::OutOfCapacity");
    }
    case ZXSlabInsertStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXSlabInsertStatus::Nullptr");
    }
    default: {
      return ZXCString::init_unsafe("ZXSlabInsertStatus::Unknown");
    }
  }
}

enum class [[nodiscard]] ZXSlabAccessStatus : zx_u8_c {
  Found = 0,
  NotFound,
  Nullptr,
  OutOfBounds,
};

static inline ZXCString zx_slab_access_status_to_cstring(
    ZXSlabAccessStatus status) {
  switch (status) {
    case ZXSlabAccessStatus::Found: {
      return ZXCString::init_unsafe("ZXSlabAccessStatus::Found");
    }
    case ZXSlabAccessStatus::NotFound: {
      return ZXCString::init_unsafe("ZXSlabAccessStatus::NotFound");
    }
    case ZXSlabAccessStatus::Nullptr: {
      return ZXCString::init_unsafe("ZXSlabAccessStatus::Nullptr");
    }
    case ZXSlabAccessStatus::OutOfBounds: {
      return ZXCString::init_unsafe("ZXSlabAccessStatus::OutOfBounds");
    }
    default: {
      return ZXCString::init_unsafe("ZXSlabAccessStatus::Unknown");
    }
  }
}

template <typename T>
struct ZXSlab {
  struct Entry_ {
    T val_;
    zx_u32 next_free_;
  };

  ZXSliceMut<Entry_> m_entries;
  zx_u32 m_first_free_entry;
  zx_u32 m_n_occupied;
  zx_u32 m_capacity;

  static inline ZXSlabInitStatus init(ZXAlloc* alloc,
                                      zx_u32 capacity,
                                      ZXSlab* slab) {
    if (alloc == nullptr || slab == nullptr) {
      return ZXSlabInitStatus::Nullptr;
    }

    ZXSliceMut<Entry_> entries;
    switch (alloc->alloc<Entry_>(capacity, &entries)) {
      case ZXAllocStatus::Ok: {
        break;
      }
      case ZXAllocStatus::OutOfMemory: {
        return ZXSlabInitStatus::OutOfMemory;
        break;
      }
      default: {
        return ZXSlabInitStatus::AllocFail;
        break;
      }
    }

    for (zx_u32 i = 0; i < capacity; i++) {
      Entry_* ptr = entries.offset_unsafe(i);
#ifdef ZX_DEV
      __builtin_memset(&ptr->val_, 0xaa, sizeof(T));
#endif
      ptr->next_free_ = zx_add_unsafe_u32(i, 1);
    }

    *slab = ZXSlab{
        .m_entries = entries,
        .m_first_free_entry = 0,
        .m_n_occupied = 0,
        .m_capacity = capacity,
    };
    return ZXSlabInitStatus::Ok;
  }

  inline void reset() {
    ZXSliceMut<Entry_> const entries = this->m_entries;
    zx_u32 const capacity = this->m_capacity;
    for (zx_u32 i = 0; i < capacity; i++) {
      Entry_* ptr = entries.offset_unsafe(i);
#ifdef ZX_DEV
      __builtin_memset(&ptr->val_, 0xaa, sizeof(T));
#endif
      ptr->next_free_ = zx_add_unsafe_u32(i, 1);
    }
  }

  inline zx_u32 n_occupied() const { return this->m_n_occupied; }

  inline zx_u32 capacity_left() const {
    return zx_sub_unsafe_u32(this->m_capacity, this->m_n_occupied);
  }

  inline ZXSlabInsertStatus insert(T const* val, zx_u32* idx) {
    if (val == nullptr || idx == nullptr) {
      return ZXSlabInsertStatus::Nullptr;
    }

    zx_u32 const key_idx = this->m_first_free_entry;

    if (key_idx == this->m_capacity) {
      *idx = ZX_U32_MAX;
      return ZXSlabInsertStatus::OutOfCapacity;
    }
    zx_assert_dev(this->m_capacity > this->m_n_occupied);
    zx_assert_dev(key_idx < this->m_entries.len());

    Entry_* entry = this->m_entries.offset_unsafe(key_idx);
    entry->val_ = *val;
    this->m_first_free_entry = entry->next_free_;
    entry->next_free_ = this->m_capacity;

    this->m_n_occupied = zx_add_unsafe_u32(this->m_n_occupied, 1);

    *idx = key_idx;
    return ZXSlabInsertStatus::Ok;
  }

  // It is invalid to call this function with a key that doesn't exist in the
  // slab
  inline ZXSlabAccessStatus remove(zx_u32 key, T* out) {
    if (out == nullptr) {
      return ZXSlabAccessStatus::Nullptr;
    }

    zx_u32 const key_idx = key;

    if (key_idx >= this->m_capacity) {
#ifdef ZX_DEV
      __builtin_memset(out, 0xaa, sizeof(T));
#endif
      return ZXSlabAccessStatus::OutOfBounds;
    }

    Entry_* entry = this->m_entries.offset_unsafe(key_idx);
    if (entry->next_free != this->m_capacity) {
      return ZXSlabAccessStatus::NotFound;
    }
    entry->next_free = this->m_first_free_entry;
    this->m_first_free_entry = key_idx;
    *out = entry->val_;
#ifdef ZX_DEV
    __builtin_memset(&entry->val_, 0xaa, sizeof(T));
#endif
    this->m_n_occupied = zx_sub_unsafe_u32(this->m_n_occupied, 1);

    return ZXSlabAccessStatus::Found;
  }

  inline ZXSlabAccessStatus get_mut(zx_u32 key, T** out) const {
    if (out == nullptr) {
      return ZXSlabAccessStatus::Nullptr;
    }

    zx_u32 const key_idx = key;

    if (key_idx >= this->m_capacity) {
      *out = nullptr;
      return ZXSlabAccessStatus::OutOfBounds;
    }

    Entry_* entry = this->m_entries.offset_unsafe(key_idx);
    if (entry->next_free != this->m_capacity) {
      *out = nullptr;
      return ZXSlabAccessStatus::NotFound;
    }

    *out = &entry->val_;
    return ZXSlabAccessStatus::Found;
  }

  inline ZXSlabAccessStatus get(zx_u32 key, T const** out) const {
    if (out == nullptr) {
      return ZXSlabAccessStatus::Nullptr;
    }
    T* ptr = nullptr;
    ZXSlabAccessStatus const status = this->get_mut(key, &ptr);
    *out = ptr;
    return status;
  }
};

#endif

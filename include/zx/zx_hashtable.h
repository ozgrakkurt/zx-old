#ifndef ZX_HASHTABLE_H
#define ZX_HASHTABLE_H

#ifndef __AVX2__
#error "avx2 is required"
#endif

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_vec.h>

enum class [[nodiscard]] ZXHashtableInitStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  OutOfMemory,
  AllocFail,
  IntegerOverflow,
  UnexpectedError,
};

static inline ZXCString
zx_hashtable_init_status_to_cstring(ZXHashtableInitStatus status) {
  switch (status) {
  case ZXHashtableInitStatus::Ok: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::Ok");
  }
  case ZXHashtableInitStatus::Nullptr: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::Nullptr");
  }
  case ZXHashtableInitStatus::OutOfMemory: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::OutOfMemory");
  }
  case ZXHashtableInitStatus::AllocFail: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::AllocFail");
  }
  case ZXHashtableInitStatus::IntegerOverflow: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::IntegerOverFlow");
  }
  case ZXHashtableInitStatus::UnexpectedError: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::UnexpectedError");
  }
  default: {
    return ZXCString::init_unsafe("ZXHashtableInitStatus::Unknown");
  }
  }
}

enum class [[nodiscard]] ZXHashtableInsertStatus : zx_u8_c {
  CreatedNewEntry = 0,
  ModifiedExistingEntry,
  OutOfCapacity,
};

static inline ZXCString
zx_hashtable_insert_status_to_cstring(ZXHashtableInsertStatus status) {
  switch (status) {
  case ZXHashtableInsertStatus::CreatedNewEntry: {
    return ZXCString::init_unsafe("ZXHashtableInsertStatus::CreatedNewEntry");
  }
  case ZXHashtableInsertStatus::ModifiedExistingEntry: {
    return ZXCString::init_unsafe(
        "ZXHashtableInsertStatus::ModifiedExistingEntry");
  }
  case ZXHashtableInsertStatus::OutOfCapacity: {
    return ZXCString::init_unsafe("ZXHashtableInsertStatus::OutOfCapacity");
  }
  default: {
    return ZXCString::init_unsafe("ZXHashtableInsertStatus::Unknown");
  }
  }
}

enum class [[nodiscard]] ZXHashtableGetOrInsertStatus : zx_u8_c {
  CreatedNewEntry = 0,
  FoundExistingEntry,
  OutOfCapacity,
  Nullptr,
};

static inline ZXCString zx_hashtable_get_or_insert_status_to_cstring(
    ZXHashtableGetOrInsertStatus status) {
  switch (status) {
  case ZXHashtableGetOrInsertStatus::CreatedNewEntry: {
    return ZXCString::init_unsafe(
        "ZXHashtableGetOrInsertStatus::CreatedNewEntry");
  }
  case ZXHashtableGetOrInsertStatus::FoundExistingEntry: {
    return ZXCString::init_unsafe(
        "ZXHashtableGetOrInsertStatus::FoundExistingEntry");
  }
  case ZXHashtableGetOrInsertStatus::OutOfCapacity: {
    return ZXCString::init_unsafe(
        "ZXHashtableGetOrInsertStatus::OutOfCapacity");
  }
  case ZXHashtableGetOrInsertStatus::Nullptr: {
    return ZXCString::init_unsafe("ZXHashtableGetOrInsertStatus::Nullptr");
  }
  default: {
    return ZXCString::init_unsafe("ZXHashtableGetOrInsertStatus::Unknown");
  }
  }
}

enum class [[nodiscard]] ZXHashtableGetStatus : zx_u8_c {
  Found = 0,
  NotFound,
  Nullptr,
};

static inline ZXCString
zx_hashtable_get_status_to_cstring(ZXHashtableGetStatus status) {
  switch (status) {
  case ZXHashtableGetStatus::Found: {
    return ZXCString::init_unsafe("ZXHashtableGetStatus::Found");
  }
  case ZXHashtableGetStatus::NotFound: {
    return ZXCString::init_unsafe("ZXHashtableGetStatus::NotFound");
  }
  case ZXHashtableGetStatus::Nullptr: {
    return ZXCString::init_unsafe("ZXHashtableGetStatus::Nullptr");
  }
  default: {
    return ZXCString::init_unsafe("ZXHashtableGetStatus::Unknown");
  }
  }
}

template <typename T, typename EqCtx,
          bool EqFn(EqCtx ctx, T const *a, T const *b)>
struct ZXHashtable {
  ZXSliceMut<T> m_entries;
  ZXSliceMut<zx_u8> m_metadata;
  zx_u32 m_cap_mask;
  zx_u32 m_capacity_left;
  zx_u32 m_capacity;

  static constexpr zx_u8 GROUP_SIZE_ = sizeof(zx_v128_u8);
  static constexpr zx_u8 EMPTY_TAG_ = 0b11111111;
  static constexpr zx_u32 LOAD_FACTOR_ = 80;

  static inline ZXHashtableInitStatus init(ZXAlloc *alloc, zx_u32 capacity,
                                           ZXHashtable *table) {
    if (alloc == nullptr || table == nullptr) {
      return ZXHashtableInitStatus::Nullptr;
    }

    bool overflow = false;
    zx_u32 cap =
        zx_next_pow_two_u32(zx_div_u32(zx_mul_u32(capacity, 100, &overflow),
                                       LOAD_FACTOR_, &overflow),
                            &overflow);
    cap = zx_max_u32(GROUP_SIZE_, cap);

    zx_u32 const cap_mask = zx_sub_u32(cap, 1, &overflow);

    // allocate extra sizeof(simd) bytes so we can do simd loads on any byte
    // index also have to mirror the first group with the extra group at the
    // end to have simple read path.
    zx_u64 const metadata_size = zx_add_u64(cap, GROUP_SIZE_, &overflow);

    if (overflow) {
      return ZXHashtableInitStatus::IntegerOverflow;
    }
    if (cap <= capacity || cap == 0 || !zx_is_pow_two_u32(cap)) {
      return ZXHashtableInitStatus::UnexpectedError;
    }

    ZXSliceMut<zx_u8> metadata;
    switch (alloc->alloc<zx_u8>(metadata_size, &metadata)) {
    case ZXAllocStatus::Ok: {
      break;
    }
    case ZXAllocStatus::OutOfMemory: {
      return ZXHashtableInitStatus::OutOfMemory;
      break;
    }
    default: {
      return ZXHashtableInitStatus::AllocFail;
      break;
    }
    }
    metadata.fill(EMPTY_TAG_);

    ZXSliceMut<T> entries;
    switch (alloc->alloc<T>(cap, &entries)) {
    case ZXAllocStatus::Ok: {
      break;
    }
    case ZXAllocStatus::OutOfMemory: {
      return ZXHashtableInitStatus::OutOfMemory;
      break;
    }
    default: {
      return ZXHashtableInitStatus::AllocFail;
      break;
    }
    }

    *table = ZXHashtable{
        .m_entries = entries,
        .m_metadata = metadata,
        .m_cap_mask = cap_mask,
        .m_capacity_left = capacity,
        .m_capacity = capacity,
    };

    return ZXHashtableInitStatus::Ok;
  }

  inline zx_u32 n_occupied() const {
    return zx_sub_unsafe_u32(this->m_capacity, this->m_capacity_left);
  }

  inline zx_u32 capacity() const { return this->m_capacity; }

  inline zx_u32 capacity_left() const { return this->m_capacity_left; }

  inline void reset() {
#ifdef ZX_DEV
    this->m_entries.memset(0xaa);
#endif
    this->m_metadata.fill(EMPTY_TAG_);
    this->m_capacity_left = this->m_capacity;
  }

  static inline zx_u32 maybe_mirror_index_(zx_u32 cap_mask, zx_u32 index) {
    return zx_wrapping_add_u32(
        (zx_wrapping_sub_u32(index, GROUP_SIZE_) & cap_mask), GROUP_SIZE_);
  }

  static inline zx_u16 tag_match_(zx_v128_u8 group, zx_v128_u8 tag_vec) {
    zx_u64_c tag_m;

    __asm__("vpcmpeqb %[tag_vec], %[group], %%xmm15\n\t"
            "vpmovmskb %%xmm15, %[tag_m]\n\t"
            : [tag_m] "=&r"(tag_m)
            : [group] "v"(group), [tag_vec] "v"(tag_vec)
            : "xmm15");

    return (zx_u16)tag_m;
  }

  static inline zx_u16 empty_match_(zx_v128_u8 group) {
    zx_u64_c empty_m;

    __asm__("vpmovmskb %[group], %[empty_m]\n\t"
            : [empty_m] "=&r"(empty_m)
            : [group] "v"(group));

    return (zx_u16)empty_m;
  }

  static inline zx_v128_u8 broadcast_(zx_u8 tag) {
    zx_v128_u8 tag_vec;
    __asm__("vmovd %[tag], %[tag_vec]\n\t"
            "vpbroadcastb %[tag_vec], %[tag_vec]\n\t"
            : [tag_vec] "=&v"(tag_vec)
            : [tag] "r"((zx_u32_c)tag));
    return tag_vec;
  }

  static inline zx_u16 remove_last_set_bit_(zx_u16 v) {
    return v & zx_wrapping_sub_u16(v, 1);
  }

  inline ZXHashtableInsertStatus insert(zx_u64 hash, T const *entry,
                                        EqCtx eqctx) {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_v128_u8 const tag_vec = broadcast_(tag);

    zx_u32 stride = 0;

    zx_u32 const cap_mask = this->m_cap_mask;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));
    while (true) {
      zx_v128_u8 const group =
          this->m_metadata.read_unaligned_unsafe<zx_v128_u8>(meta_idx);

      zx_u16 tag_match = tag_match_(group, tag_vec);
      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = zx_add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

        T *entry_p = this->m_entries.offset_unsafe(index);
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          *entry_p = *entry;
          return ZXHashtableInsertStatus::ModifiedExistingEntry;
        } else {
          tag_match = remove_last_set_bit_(tag_match);
        }
      }

      zx_u16 const empty_match = empty_match_(group);
      if (empty_match != 0) [[likely]] {
        zx_u8 const tag_pos = zx_ctz_u16(empty_match);

        zx_u32 const index = zx_add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask, index);
        zx_u32 const cap_left = this->m_capacity_left;
        if (cap_left == 0) {
          return ZXHashtableInsertStatus::OutOfCapacity;
        }
        ZXSliceMut<zx_u8> metadata = this->m_metadata;

        metadata.write_unsafe(index, tag);
        metadata.write_unsafe(maybe_mirror_idx, tag);
        this->m_capacity_left = zx_sub_unsafe_u32(cap_left, 1);
        this->m_entries.write_unsafe(index, *entry);

        return ZXHashtableInsertStatus::CreatedNewEntry;
      }

      // Should have found an empty slot by now
      zx_assert_dev(stride <= cap_mask);

      stride = zx_add_unsafe_u32(stride, GROUP_SIZE_);
      meta_idx = zx_add_unsafe_u32(meta_idx, stride) & cap_mask;
    }
  }

  inline ZXHashtableGetOrInsertStatus
  get_or_insert(zx_u64 hash, T const *entry, EqCtx eqctx, T const **out) {
    if (out == nullptr) {
      return ZXHashtableGetOrInsertStatus::Nullptr;
    }

    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask = this->m_cap_mask;
    zx_v128_u8 const tag_vec = broadcast_(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

    while (true) {
      zx_v128_u8 const group =
          this->m_metadata.read_unaligned_unsafe<zx_v128_u8>(meta_idx);

      zx_u16 tag_match = tag_match_(group, tag_vec);
      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = zx_add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

        T *entry_p = this->m_entries.offset_unsafe(index);
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          *out = entry_p;
          return ZXHashtableGetOrInsertStatus::FoundExistingEntry;
        } else {
          tag_match = remove_last_set_bit_(tag_match);
        }
      }

      zx_u16 const empty_match = empty_match_(group);
      if (empty_match != 0) [[likely]] {
        zx_u8 const tag_pos = zx_ctz_u16(empty_match);
        zx_u32 const index = zx_add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask, index);
        zx_u32 const cap_left = this->m_capacity_left;
        if (cap_left == 0) {
          *out = nullptr;
          return ZXHashtableGetOrInsertStatus::OutOfCapacity;
        }
        ZXSliceMut<zx_u8> metadata = this->m_metadata;
        T *entry_p = this->m_entries.offset_unsafe(index);

        *entry_p = *entry;
        metadata.write_unsafe(index, tag);
        metadata.write_unsafe(maybe_mirror_idx, tag);
        this->m_capacity_left = zx_sub_unsafe_u32(cap_left, 1);

        *out = entry_p;
        return ZXHashtableGetOrInsertStatus::CreatedNewEntry;
      }

      // Should have found an empty slot by now
      zx_assert_dev(stride <= cap_mask);

      stride = zx_add_unsafe_u32(stride, GROUP_SIZE_);
      meta_idx = zx_add_unsafe_u32(meta_idx, stride) & cap_mask;
    }
  }

  inline ZXHashtableGetStatus get(zx_u64 const hash, T const *entry,
                                  EqCtx eqctx, T const **out) const {
    if (out == nullptr) {
      return ZXHashtableGetStatus::Nullptr;
    }

    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask = this->m_cap_mask;
    zx_v128_u8 const tag_vec = broadcast_(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

    while (true) {
      zx_v128_u8 const group =
          this->m_metadata.read_unaligned_unsafe<zx_v128_u8>(meta_idx);

      zx_u16 tag_match = tag_match_(group, tag_vec);
      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = zx_add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

        T const *entry_p = this->m_entries.offset_unsafe(index);
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          *out = entry_p;
          return ZXHashtableGetStatus::Found;
        } else {
          tag_match = remove_last_set_bit_(tag_match);
        }
      }

      zx_u16 const empty_match = empty_match_(group);
      if (empty_match != 0) [[likely]] {
        *out = nullptr;
        return ZXHashtableGetStatus::NotFound;
      }

      // Should have found an empty slot by now
      zx_assert_dev(stride <= cap_mask);

      stride = zx_add_unsafe_u32(stride, GROUP_SIZE_);
      meta_idx = zx_add_unsafe_u32(meta_idx, stride) & cap_mask;
    }
  }
}; // struct ZXHashtable

#endif

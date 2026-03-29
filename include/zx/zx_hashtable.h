#ifndef ZX_HASHTABLE_H
#define ZX_HASHTABLE_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>
#include <zx/zx_vec.h>

template <typename T, typename EqCtx,
          bool EqFn(EqCtx ctx, T const *a, T const *b)>
struct ZXHashtable {
  T *entries;
  zx_u8 *metadata;
  zx_u32 cap_mask;
  zx_u32 capacity_left_;
  zx_u32 capacity;

  static constexpr zx_u8 GROUP_SIZE_ = sizeof(zx_v128_u8);
  static constexpr zx_u8 EMPTY_TAG_ = 0b11111111;
  static constexpr zx_u32 LOAD_FACTOR_ = 80;

  static inline ZXStatus init(ZXAlloc *alloc, zx_u32 capacity,
                              ZXHashtable *table, ZXErrStack *err_stack) {
    ZX_ASSERT((alloc != nullptr));
    ZX_ASSERT((table != nullptr));
    ZX_ASSERT((err_stack != nullptr));

    zx_u32 cap = zx_next_pow_two_u32(capacity * (zx_u32){100} / LOAD_FACTOR_);
    cap = zx_int_max_u32(GROUP_SIZE_, cap);
    ZX_ASSERT((cap > capacity));
    ZX_ASSERT((cap > 0));
    // check it is a power of two
    ZX_ASSERT((zx_is_pow_two_u32(cap)));
    zx_u32 const cap_mask_ = zx_chk_sub_u32(cap, 1);

    // allocate extra sizeof(simd) bytes so we can do simd loads on any byte
    // index also have to mirror the first group with the extra group at the end
    // to have simple read path.
    zx_u32 const metadata_size = zx_chk_add_u32(cap, GROUP_SIZE_);
    zx_u8 *metadata = nullptr;
    ZX_ERR_TRY((alloc->alloc<zx_u8>(metadata_size, &metadata, err_stack)),
               err_stack);
    __builtin_memset(metadata, EMPTY_TAG_, metadata_size);

    T *entries = nullptr;
    ZX_ERR_TRY((alloc->alloc<T>(cap, &entries, err_stack)), err_stack);

    *table = ZXHashtable{
        .entries = entries,
        .metadata = metadata,
        .cap_mask = cap_mask_,
        .capacity_left_ = capacity,
        .capacity = capacity,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 capacity_left() const {
    return this->capacity_left_;
  }

  inline void reset() {
    zx_u32 const cap = zx_chk_add_u32(this->cap_mask, 1);
    zx_u32 const metadata_size = zx_chk_add_u32(cap, GROUP_SIZE_);
    __builtin_memset(this->metadata, EMPTY_TAG_, metadata_size);
    this->capacity_left_ = this->capacity;
  }

  __attribute__((always_inline)) static inline zx_u32
  maybe_mirror_index_(zx_u32 cap_mask_, zx_u32 index) {
    return ((index - GROUP_SIZE_) & cap_mask_) + GROUP_SIZE_;
  }

  // Returns true if a matching entry existed before and was overwritten.
  //
  // Returns false otherwise.
  //
  // This function will abort with an error if the table is already full and a
  // matching entry doesn't exist in the table already.
  inline bool insert(zx_u64 hash, T const *entry, EqCtx eqctx) {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask_ = this->cap_mask;
    zx_v128_u8 const tag_vec = zx_v128_broadcast_u8(tag);

    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask_ & ((zx_u32)(hash));

    zx_u8 const *meta = this->metadata;
    while (true) {
      zx_v128_u8 const group = zx_v128_loadu_u8(&meta[meta_idx]);

      zx_u16 tag_match = zx_v128_movmsk_u8(zx_v128_cmpeq_u8(tag_vec, group));
      zx_u16 const empty_match = zx_v128_movmsk_u8(group);

      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask_;

        T *entry_p = &this->entries[index];
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          ZX_FUZZ_MARKER;

          *entry_p = *entry;
          return true;
        } else {
          ZX_FUZZ_MARKER;

          tag_match = tag_match & (tag_match - (zx_u16){1});
        }
      }

      if (empty_match != 0) [[likely]] {
        ZX_FUZZ_MARKER;

        zx_u8 const tag_pos = zx_ctz_u16(empty_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask_;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask_, index);
        zx_u32 const cap_left = this->capacity_left_;
        ZX_ASSERT((cap_left > 0));
        zx_u8 *metas = this->metadata;

        this->capacity_left_ = cap_left - (zx_u32){1};
        this->entries[index] = *entry;
        metas[index] = tag;
        metas[maybe_mirror_idx] = tag;

        return false;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask_));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask_;

      ZX_FUZZ_MARKER;
    }
  }

  // Returns a pointer to the pre-existing entry if a matching entry is found.
  // Inserts the new entry and returns a pointer to that if a matching entry
  // isn't found.
  //
  // In any case the returned pointer will point into the table allocation and
  // not the provided entry pointer.
  //
  // This function will abort with an error if the table is already full and a
  // matching entry doesn't exist in the table already.
  inline T const *get_or_insert(zx_u64 hash, T const *entry, EqCtx eqctx) {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask_ = this->cap_mask;
    zx_v128_u8 const tag_vec = zx_v128_broadcast_u8(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask_ & ((zx_u32)(hash));

    zx_u8 const *meta = this->metadata;
    while (true) {

      zx_v128_u8 const group = zx_v128_loadu_u8(&meta[meta_idx]);

      zx_u16 tag_match = zx_v128_movmsk_u8(zx_v128_cmpeq_u8(tag_vec, group));
      zx_u16 const empty_match = zx_v128_movmsk_u8(group);

      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask_;

        T *entry_p = &this->entries[index];
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          ZX_FUZZ_MARKER;
          return entry_p;
        } else {
          ZX_FUZZ_MARKER;

          tag_match = tag_match & (tag_match - (zx_u16){1});
        }
      }

      if (empty_match != 0) [[likely]] {
        ZX_FUZZ_MARKER;

        zx_u8 const tag_pos = zx_ctz_u16(empty_match);
        zx_u32 const index = (meta_idx + tag_pos) & cap_mask_;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask_, index);
        zx_u32 const cap_left = this->capacity_left_;
        ZX_ASSERT((cap_left > 0));
        zx_u8 *metas = this->metadata;
        T *entry_p = &this->entries[index];

        this->capacity_left_ = cap_left - (zx_u32){1};
        *entry_p = *entry;
        metas[index] = tag;
        metas[maybe_mirror_idx] = tag;

        return entry_p;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask_));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask_;

      ZX_FUZZ_MARKER;
    }
  }

  // returns a pointer to the entry if it is found in the table.
  // returns nullptr if a matching entry isn't found.
  inline T const *get(zx_u64 const hash, T const *entry, EqCtx eqctx) const {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask_ = this->cap_mask;
    zx_v128_u8 const tag_vec = zx_v128_broadcast_u8(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask_ & ((zx_u32)(hash));

    zx_u8 const *meta = this->metadata;
    while (true) {
      zx_v128_u8 const group = zx_v128_loadu_u8(&meta[meta_idx]);

      zx_u16 tag_match = zx_v128_movmsk_u8(zx_v128_cmpeq_u8(tag_vec, group));
      zx_u16 const empty_match = zx_v128_movmsk_u8(group);

      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask_;

        T const *entry_p = &this->entries[index];
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          ZX_FUZZ_MARKER;
          return entry_p;
        } else {
          ZX_FUZZ_MARKER;

          tag_match = tag_match & (tag_match - (zx_u16){1});
        }
      }

      if (empty_match != 0) [[likely]] {
        ZX_FUZZ_MARKER;
        return nullptr;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask_));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask_;

      ZX_FUZZ_MARKER;
    }
  }
}; // struct ZXHashtable

#endif

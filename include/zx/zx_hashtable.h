#ifndef ZX_HASHTABLE_H
#define ZX_HASHTABLE_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

template <typename T, typename EqCtx,
          bool EqFn(EqCtx ctx, T const *a, T const *b)>
struct ZXHashtable {
  ZXSliceMut<T> m_entries;
  ZXSliceMut<zx_u8> m_metadata;
  zx_u32 m_cap_mask;
  zx_u32 m_capacity_left;
  zx_u32 m_capacity;

  typedef zx_u8 vect_ __attribute__((vector_size(16)));
  typedef unsigned long regt;
  static_assert(sizeof(regt) == sizeof(void *));

  static constexpr zx_u8 GROUP_SIZE_ = sizeof(vect_);
  static constexpr zx_u8 EMPTY_TAG_ = 0b11111111;
  static constexpr zx_u32 LOAD_FACTOR_ = 80;

  static inline ZXStatus init(ZXAlloc *alloc, zx_u32 capacity,
                              ZXHashtable *table, ZXErrStack *err_stack) {
    ZX_ASSERT((table != nullptr));
    ZX_ASSERT((err_stack != nullptr));

    zx_u32 cap = zx_next_pow_two_u32(capacity * (zx_u32){100} / LOAD_FACTOR_);
    cap = zx_int_max_u32(GROUP_SIZE_, cap);
    ZX_ASSERT((cap > capacity));
    ZX_ASSERT((cap > 0));
    // check it is a power of two
    ZX_ASSERT((zx_is_pow_two_u32(cap)));
    zx_u32 const cap_mask = zx_chk_sub_u32(cap, 1);

    // allocate extra sizeof(simd) bytes so we can do simd loads on any byte
    // index also have to mirror the first group with the extra group at the
    // end to have simple read path.
    zx_u32 const metadata_size = zx_chk_add_u32(cap, GROUP_SIZE_);
    ZXSliceMut<zx_u8> metadata;
    ZX_ERR_TRY((alloc->alloc<zx_u8>(metadata_size, &metadata, err_stack)),
               err_stack);
    metadata.memset(EMPTY_TAG_);

    ZXSliceMut<T> entries;
    ZX_ERR_TRY((alloc->alloc<T>(cap, &entries, err_stack)), err_stack);
#ifdef ZX_MEMORY_SAFETY_ENABLED
    entries.memset(0xf0);
#endif

    *table = ZXHashtable{
        .m_entries = entries,
        .m_metadata = metadata,
        .m_cap_mask = cap_mask,
        .m_capacity_left = capacity,
        .m_capacity = capacity,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 capacity_left() const {
    return this->m_capacity_left;
  }

  inline void reset() {
    this->m_metadata.memset(EMPTY_TAG_);
    this->m_capacity_left = this->m_capacity;
  }

  __attribute__((always_inline)) static inline zx_u32
  maybe_mirror_index_(zx_u32 cap_mask, zx_u32 index) {
    return ((index - GROUP_SIZE_) & cap_mask) + GROUP_SIZE_;
  }

  __attribute__((always_inline)) static inline void load_(void const *group_ptr,
                                                          vect_ tag_vec,
                                                          zx_u16 *tag_match,
                                                          zx_u16 *empty_match) {
    regt tag_m;
    regt empty_m;

    __asm__("vmovdqu %[group_ptr], %%xmm1\n\t"        // load group
            "vpcmpeqb %[tag_vec], %%xmm1, %%xmm0\n\t" // tag match
            "vpmovmskb %%xmm0, %[tag_m]\n\t"
            "vpmovmskb %%xmm1, %[empty_m]\n\t" // empty match
            : [tag_m] "=r"(tag_m), [empty_m] "=r"(empty_m)
            : [group_ptr] "m"(*(zx_u8 const(*)[GROUP_SIZE_])group_ptr),
              [tag_vec] "v"(tag_vec)
            : "xmm0", "xmm1");

    *tag_match = (zx_u16)tag_m;
    *empty_match = (zx_u16)empty_m;
  }

  __attribute__((always_inline)) static inline zx_u16
  tag_match_(vect_ group, vect_ tag_vec) {
    regt tag_m;

    __asm__("vpcmpeqb %[tag_vec], %[group], %%xmm8\n\t"
            "vpmovmskb %%xmm8, %[tag_m]\n\t"
            : [tag_m] "=r"(tag_m)
            : [group] "v"(group), [tag_vec] "v"(tag_vec)
            : "xmm8");

    return (zx_u16)tag_m;
  }

  __attribute__((always_inline)) static inline zx_u16
  empty_match_(vect_ group) {
    regt empty_m;

    __asm__("vpmovmskb %[group], %[empty_m]\n\t"
            : [empty_m] "=r"(empty_m)
            : [group] "v"(group));

    return (zx_u16)empty_m;
  }

  __attribute__((always_inline)) static inline vect_
  load_group_(void const *group_ptr) {
    vect_ group;
    __asm__("vmovdqu %[group_ptr], %[group]\n\t"
            : [group] "=v"(group)
            : [group_ptr] "m"(*(zx_u8 const(*)[GROUP_SIZE_])group_ptr));

    return group;
  }

  __attribute__((always_inline)) static inline vect_ broadcast_(zx_u8 tag) {
    vect_ tag_vec;
    __asm__("vmovd %1, %0\n\t"
            "vpbroadcastb %0, %0\n\t"
            : "=v"(tag_vec)
            : "r"((regt)tag));
    return tag_vec;
  }

  // Returns true if a matching entry existed before and was overwritten.
  //
  // Returns false otherwise.
  //
  // This function will abort with an error if the table is already full and a
  // matching entry doesn't exist in the table already.
  inline bool insert(zx_u64 hash, T const *entry, EqCtx eqctx) {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    vect_ const tag_vec = broadcast_(tag);

    zx_u32 stride = 0;

    zx_u32 const cap_mask = this->m_cap_mask;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));
    while (true) {
      zx_u16 tag_match;
      zx_u16 empty_match;
      load_(this->m_metadata.index(meta_idx), tag_vec, &tag_match,
            &empty_match);

      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask;

        T *entry_p = this->m_entries.index(index);
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

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask, index);
        zx_u32 const cap_left = this->m_capacity_left;
        ZX_ASSERT((cap_left > 0));
        ZXSlice<zx_u8> metadata = this->m_metadata;

        *metadata.index(index) = tag;
        *metadata.index(maybe_mirror_idx) = tag;
        this->m_capacity_left = cap_left - (zx_u32){1};
        *this->m_entries.index(index) = *entry;

        return false;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask;

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
    zx_u32 const cap_mask = this->m_cap_mask;
    vect_ const tag_vec = broadcast_(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

    while (true) {
      zx_u16 tag_match;
      zx_u16 empty_match;
      load_(this->m_metadata.index(meta_idx), tag_vec, &tag_match,
            &empty_match);

      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask;

        T *entry_p = this->m_entries.index(index);
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
        zx_u32 const index = (meta_idx + tag_pos) & cap_mask;

        zx_u32 const maybe_mirror_idx = maybe_mirror_index_(cap_mask, index);
        zx_u32 const cap_left = this->m_capacity_left;
        ZX_ASSERT((cap_left > 0));
        ZXSliceMut<zx_u8> metadata = this->m_metadata;
        T *entry_p = this->m_entries.index(index);

        *entry_p = *entry;
        *metadata.index(index) = tag;
        *metadata.index(maybe_mirror_idx) = tag;
        this->m_capacity_left = cap_left - (zx_u32){1};

        return entry_p;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask;

      ZX_FUZZ_MARKER;
    }
  }

#define ZX_HASHTABLE_GET(T, htable, hash, eqctx) {}

  // returns a pointer to the entry if it is found in the table.
  // returns nullptr if a matching entry isn't found.
  inline T const *get(zx_u64 const hash, T const *entry, EqCtx eqctx) const {
    zx_u8 const tag = (zx_u8)(hash >> 57);
    zx_u32 const cap_mask = this->cap_mask;
    vect_ const tag_vec = broadcast_(tag);
    zx_u32 stride = 0;

    zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

    while (true) {
      vect_ const group = load_group_(this->m_metadata.index(meta_idx));

      zx_u16 tag_match = tag_match_(group, tag_vec);
      while (tag_match != 0) {
        zx_u8 const tag_pos = zx_ctz_u16(tag_match);

        zx_u32 const index = (meta_idx + tag_pos) & cap_mask;

        T const *entry_p = this->m_entries.index(index);
        if (EqFn(eqctx, entry_p, entry)) [[likely]] {
          ZX_FUZZ_MARKER;
          return entry_p;
        } else {
          ZX_FUZZ_MARKER;

          tag_match = tag_match & (tag_match - (zx_u16){1});
        }
      }

      zx_u16 const empty_match = empty_match_(group);
      if (empty_match != 0) [[likely]] {
        ZX_FUZZ_MARKER;
        return nullptr;
      }

      // Should have found an empty slot by now
      ZX_DBG_ASSERT((stride <= cap_mask));

      stride += GROUP_SIZE_;
      meta_idx = (meta_idx + stride) & cap_mask;

      ZX_FUZZ_MARKER;
    }
  }
}; // struct ZXHashtable

#endif

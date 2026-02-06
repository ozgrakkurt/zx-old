#ifndef SHIN_HASHTABLE_H
#define SHIN_HASHTABLE_H

#include <immintrin.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <stdbit.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// This code is mostly ported from Rust crate hashbrown

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE2__
#error "sse2 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

typedef struct ShinHashtable {
  void *entries;
  shin_u8 *metadata;
  // Mask to get a group(simd size) index from a hash value. The value is one
  // less than the total number of groups in the table. The total number of
  // groups in the table should always be a power of two so we should be able to
  // bitwise AND this mask with an index value for modulo operation.
  shin_u32 group_idx_mask;
  // Number of vacant entries in the table.
  shin_u32 capacity_left;
} ShinHashtable;

_Static_assert(sizeof(ShinHashtable) == sizeof(void *) * 3);

constexpr shin_u8 SHIN_HASHTABLE_TAG_EMPTY = 0b11111111wbu;
constexpr shin_u8 SHIN_HASHTABLE_TAG_DELETED = 0b10000000wbu;

static inline shin_u32 ShinHashtable_calculate_cap_(shin_u32 capacity) {
  // 80% load factor
  shin_u32 cap = capacity * (shin_u32){100wbu} / (shin_u32){80wbu};
  cap = stdc_bit_ceil(SHIN_NEXT_MULTIPLE_OF(cap, sizeof(__m128i)));

  SHIN_ASSERT((cap >= capacity));
  SHIN_ASSERT((cap > 0));
  // check it is a power of two
  SHIN_ASSERT((stdc_has_single_bit(cap)));

  return cap;
}

static inline ShinStatus ShinHashtable_init(ShinAlloc *alloc, shin_u32 capacity,
                                            shin_usize entry_align,
                                            shin_u32 entry_size,
                                            ShinHashtable *table,
                                            ShinErrStack *err_stack) {
  shin_u32 const cap = ShinHashtable_calculate_cap_(capacity);
  SHIN_ASSERT((cap % sizeof(__m128i) == 0));
  shin_u32 const num_groups = cap / sizeof(__m128i);
  // check it is a power of two
  SHIN_ASSERT((stdc_has_single_bit(num_groups)));
  shin_u32 const group_idx_mask = num_groups - 1;

  // alignment for cache line
  constexpr shin_usize ALIGN = 64;

  shin_usize const metadata_size = cap * sizeof(shin_u8);
  // need to align to at least the simd size so we can use unaligned
  // instructions
  shin_usize const metadata_align = SHIN_MAX((sizeof(__m128i)), ALIGN);
  shin_u8 *metadata = nullptr;
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(alloc, metadata_align, metadata_size,
                                        (void **)&metadata, err_stack)),
               err_stack);
  memset(metadata, SHIN_HASHTABLE_TAG_EMPTY, metadata_size);

  shin_usize const entries_size = (shin_usize){entry_size} * (shin_usize){cap};
  shin_usize const entries_align = SHIN_MAX(entry_align, ALIGN);
  void *entries = nullptr;
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(alloc, entries_align, entries_size,
                                        &entries, err_stack)),
               err_stack);

  *table = (ShinHashtable){
      .entries = entries,
      .metadata = metadata,
      .group_idx_mask = group_idx_mask,
      .capacity_left = cap,
  };

  return ShinStatus_Ok;
}

static inline shin_u16 ShinHashtable_match_tag_(__m128i const group,
                                                shin_u8 const tag) {
  __m128i const cmp = _mm_cmpeq_epi8(group, _mm_set1_epi8((shin_i8)tag));
  return (shin_u16)_mm_movemask_epi8(cmp);
}

static inline shin_u16
ShinHashtable_match_empty_or_deleted_(__m128i const group) {
  return (shin_u16)_mm_movemask_epi8(group);
}

// function for checking if keys of two entries are equal
typedef bool key_eqfn(void const *ctx, void const *entry_a,
                      void const *entry_b) shin_hashtable_key_eqfn;

static inline bool
ShinHashtable_insert(ShinHashtable *const table, shin_u64 const hash,
                     void const *const entry, shin_u32 const entry_size,
                     void const *const ctx,
                     shin_hashtable_key_eqfn const key_eqfn) {
  shin_u32 const group_idx_mask = table->group_idx_mask;
  shin_u32 probe = 1;

  shin_u8 const tag = (shin_u8)(hash >> 57);
  shin_u32 group_idx = group_idx_mask & ((shin_u32)(hash));

  __m128i *restrict const groups = (__m128i *)table->metadata;
  while (true) {
    __m128i const group = groups[group_idx];
    shin_u16 const tag_match = ShinHashtable_match_tag_(group, tag);

    while (tag_match != 0) {
      // check if position has eq key
      // if(key_eqfn()) {
      //   set_metadata_ to tag;
      //   memcpy(slot, entry, entry_size);
      //   return true;
      // }
    }

    shin_u16 const empty_match = ShinHashtable_match_empty_or_deleted_(group);

    // if (empty_match != 0) {
    //   ASSERT((capacity_left > 0));
    //   capacity_left -= 1;
    //   set_metadata_ to tag;
    //   memcpy(slot, entry, entrysize);
    //   return false;
    // }

    group_idx = (group_idx + probe) & group_idx_mask;
    probe += 1;
  }
}

static inline bool
ShinHashtable_remove(ShinHashtable *const table, shin_u64 const hash,
                     void const *const entry, void const *const ctx,
                     shin_hashtable_key_eqfn const key_eqfn) {
  shin_u32 const group_idx_mask = table->group_idx_mask;
  shin_u32 probe = 1;

  shin_u8 const tag = (shin_u8)(hash >> 57);
  shin_u32 group_idx = group_idx_mask & ((shin_u32)(hash));

  __m128i *restrict const groups = (__m128i *)table->metadata;
  while (true) {
    __m128i const group = groups[group_idx];
    shin_u16 const tag_match = ShinHashtable_match_tag_(group, tag);

    while (tag_match != 0) {
      // check if position has eq key
      // if(key_eqfn()) {
      //    set metadata to empty
      //   return true;
      // }
    }

    // maybe different
    shin_u16 const empty_match = ShinHashtable_match_empty_or_deleted_(group);

    // if (empty_match != 0) {
    //   ASSERT((capacity_left > 0));
    //   capacity_left -= 1;
    //   memcpy(slot, entry, entrysize);
    //   return false;
    // }

    group_idx = (group_idx + probe) & group_idx_mask;
    probe += 1;
  }
}

// static inline bool
// ShinHashtable_delete(ShinHashtable *const table, shin_u64 hash,
//                      void const *const entry, void const *const ctx,
//                      shin_hashtable_key_eqfn const key_eqfn) {}

// typedef struct ShinHashtableSlot {
//   void const *const entry;
//   bool found;
// } ShinHashtableSlot;
//
// static inline ShinHashtableSlot
// ShinHashtable_get(ShinHashtable const *const table, shin_u64 hash, )

#ifdef __cplusplus
}
#endif

#endif

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
  shin_u32 cap_mask;
  shin_u32 capacity_left;
} ShinHashtable;

_Static_assert(sizeof(ShinHashtable) == sizeof(void *) * 3);

constexpr shin_u8 SHIN_HASHTABLE_TAG_EMPTY = 0b11111111wbu;

static inline ShinStatus ShinHashtable_init(ShinAlloc *alloc, shin_u32 capacity,
                                            shin_usize entry_align,
                                            shin_u32 entry_size,
                                            ShinHashtable *table,
                                            ShinErrStack *err_stack) {
  shin_u32 cap =
      stdc_bit_ceil(capacity * (shin_u32){100wbu} / (shin_u32){80wbu});
  cap = SHIN_MAX((shin_u32){sizeof(__m128i)}, cap);
  SHIN_ASSERT((cap >= capacity));
  SHIN_ASSERT((cap > 0));
  // check it is a power of two
  SHIN_ASSERT((stdc_has_single_bit(cap)));
  shin_u32 const cap_mask = cap - 1;

  // align to cachline because why not?
  constexpr shin_usize ALIGN = 64;

  // allocate extra sizeof(simd) bytes so we can do simd loads on any byte index
  // also have to mirror the first group with the extra group at the end to have
  // simple read path.
  shin_usize const metadata_size = cap + sizeof(__m128i);
  shin_u8 *metadata = nullptr;
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(alloc, ALIGN, metadata_size,
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
      .cap_mask = cap_mask,
      .capacity_left = cap,
  };

  return ShinStatus_Ok;
}

static inline shin_u16 ShinHashtable_match_tag_(__m128i const group,
                                                shin_u8 const tag) {
  __m128i const cmp = _mm_cmpeq_epi8(group, _mm_set1_epi8((shin_i8)tag));
  return (shin_u16)_mm_movemask_epi8(cmp);
}

static inline shin_u16 ShinHashtable_match_empty_(__m128i const group) {
  return (shin_u16)_mm_movemask_epi8(group);
}

static inline shin_u16 ShinHashtable_match_full_(__m128i const group) {
  return ~(ShinHashtable_match_empty_(group));
}

static inline shin_u16 ShinHashtable_remove_lowest_set_bit_(shin_u16 bitmask) {
  return bitmask & (bitmask - 1);
}

static inline shin_u32 ShinHashtable_maybe_mirror_index_(shin_u32 cap_mask,
                                                         shin_u32 index) {
  constexpr shin_u32 GROUP_SIZE = sizeof(__m128i);
  return ((index - GROUP_SIZE) & cap_mask) + GROUP_SIZE;
}

// function for checking if keys of two entries are equal
typedef bool shin_hashtable_key_eqfn(void const *ctx, void const *entry_a,
                                     void const *entry_b);

static inline bool
ShinHashtable_insert(ShinHashtable *const table, shin_u64 const hash,
                     void const *const entry, shin_u32 const entry_size,
                     void const *const ctx, shin_hashtable_key_eqfn key_eqfn) {
  shin_u32 const cap_mask = table->cap_mask;
  shin_u32 stride = 0;

  shin_u8 const tag = (shin_u8)(hash >> 57);
  shin_u32 meta_idx = cap_mask & ((shin_u32)(hash));

  shin_u8 *restrict const meta = table->metadata;
  while (true) {
    __m128i const group = _mm_loadu_epi8(&meta[meta_idx]);

    {
      shin_u16 tag_match = ShinHashtable_match_tag_(group, tag);

      while (tag_match != 0) {
        shin_u32 const tag_pos = stdc_trailing_zeros(tag_match);

        shin_u32 const idx = (meta_idx + tag_pos) & cap_mask;
        shin_u32 const maybe_mirror_idx =
            ShinHashtable_maybe_mirror_index_(cap_mask, meta_idx + tag_pos);

        shin_u8 *const entries = (shin_u8 *)table->entries;
        void *const entry_ptr = &entries[entry_size * idx];
        if (key_eqfn(ctx, entry_ptr, entry)) {
          meta[idx] = tag;
          meta[maybe_mirror_idx] = tag;
          memcpy(entry_ptr, entry, entry_size);
          return true;
        } else {
          tag_match = ShinHashtable_remove_lowest_set_bit_(tag_match);
        }
      }
    }

    {
      shin_u16 const empty_match = ShinHashtable_match_empty_(group);

      if (empty_match != 0) {
        SHIN_ASSERT((table->capacity_left > 0));
        table->capacity_left -= 1;

        shin_u32 const tag_pos = stdc_trailing_zeros(empty_match);

        shin_u32 const idx = (meta_idx + tag_pos) & cap_mask;
        shin_u32 const maybe_mirror_idx =
            ShinHashtable_maybe_mirror_index_(cap_mask, meta_idx + tag_pos);

        meta[idx] = tag;
        meta[maybe_mirror_idx] = tag;

        shin_u8 *entries = (shin_u8 *)table->entries;
        void *const entry_ptr = &entries[entry_size * idx];
        memcpy(entry_ptr, entry, entry_size);

        return false;
      }
    }

    stride += sizeof(__m128i);
    meta_idx = (meta_idx + stride) & cap_mask;

    // Should have found an empty bucket by now
    SHIN_ASSERT((stride <= cap_mask));
  }
}

#ifdef __cplusplus
}
#endif

#endif

#ifndef SHIN_HASHTABLE_H
#define SHIN_HASHTABLE_H

#include <immintrin.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE2__
#error "sse2 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

typedef shin_u16 ShinHashtableBitMaskWord;

constexpr shin_usize SHIN_HASHTABLE_BIT_MASK_STRIDE = 1;
constexpr ShinHashtableBitMaskWord SHIN_HASHTABLE_BIT_MASK_ITER_MASK =
    ~((ShinHashtableBitMaskWord){1wbu});

typedef shin_u8 ShinHashtableTag;
static constexpr ShinHashtableTag SHIN_HASHTABLE_TAG_EMPTY = 0b11111111wbu;
static constexpr ShinHashtableTag SHIN_HASHTABLE_TAG_DELETED = 0b10000000wbu;

static inline bool ShinHashtableTag_is_full(ShinHashtableTag tag) {
  return (tag & (shin_u8){0x80wbu}) == 0;
}

static inline bool ShinHashtableTag_is_special(ShinHashtableTag tag) {
  return (tag & (shin_u8){0x80wbu}) != 0;
}

static inline bool ShinHashtableTag_special_is_empty(ShinHashtableTag tag) {
  SHIN_DBG_ASSERT((ShinHashtableTag_is_special(tag)));
  return (tag & (shin_u8){0x01}) != 0;
}

// Create a "full" tag using given hash
static inline ShinHashtableTag ShinHashtableTag_full(shin_u64 hash) {
  constexpr shin_usize HASH_NBITS = sizeof(shin_u64) * 8;

  shin_u8 const top7 = (shin_u8)(hash >> (HASH_NBITS - 7));
  return top7 & ((shin_u8){0x7fwbu});
}

static inline void ShinHashtableTag_fill(ShinHashtableTag tag,
                                         ShinHashtableTag *slice,
                                         shin_usize len) {
  for (shin_usize i = 0; i < len; i++) {
    slice[i] = tag;
  }
}

static inline void ShinHashtableTag_fill_empty(ShinHashtableTag *slice,
                                               shin_usize len) {
  for (shin_usize i = 0; i < len; i++) {
    slice[i] = SHIN_HASHTABLE_TAG_EMPTY;
  }
}

typedef __m128i ShinHashtableGroup;

static constexpr shin_usize SHIN_HASHTABLE_GROUP_WIDTH =
    sizeof(ShinHashtableGroup);

static inline ShinHashtableGroup
ShinHashtableGroup_load(ShinHashtableTag const *ptr) {
  return _mm_loadu_si128((ShinHashtableGroup const *)ptr);
}

static inline ShinHashtableGroup
ShinHashtableGroup_load_aligned(ShinHashtableTag const *ptr) {
  SHIN_DBG_ASSERT((((shin_usize)ptr) % alignof(ShinHashtableGroup) == 0));
  return _mm_load_si128((ShinHashtableGroup const *)ptr);
}

static inline void ShinHastableGroup_store_aligned(ShinHashtableGroup group,
                                                   ShinHashtableTag *ptr) {
  SHIN_DBG_ASSERT((((shin_usize)ptr) % alignof(ShinHashtableGroup) == 0));
  _mm_store_si128((ShinHashtableGroup *)ptr, group);
}

static inline ShinHashtableBitMaskWord
ShinHashtableGroup_match_tag(ShinHashtableGroup group, ShinHashtableTag tag) {
  ShinHashtableGroup const cmp =
      _mm_cmpeq_epi8(group, _mm_set1_epi8((shin_i8){tag}));
  return (shin_u16)_mm_movemask_epi8(cmp);
}

static inline ShinHashtableBitMaskWord
ShinHashtableGroup_match_empty(ShinHashtableGroup group) {
  return ShinHashtableGroup_match_tag(group, SHIN_HASHTABLE_TAG_EMPTY);
}

#ifdef __cplusplus
}
#endif

#endif

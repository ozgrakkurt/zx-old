#ifndef ZX_HASHTABLE_H
#define ZX_HASHTABLE_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __AVX2__
#error "avx2 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

typedef struct ZXHashtable {
  void *entries;
  zx_u8 *metadata;
  zx_u32 cap_mask;
  zx_u32 capacity_left;
} ZXHashtable;

static_assert(sizeof(ZXHashtable) == sizeof(void *) * 3);

static inline zx_u32 ZXHashtable_capacity_left(ZXHashtable const *table) {
  return table->capacity_left;
}

// size of xmm simd register
constexpr zx_u8 ZX_HASHTABLE_GROUP_SIZE_ = 16;

constexpr zx_u8 ZX_HASHTABLE_TAG_EMPTY = 0b11111111wbu;

constexpr zx_u32 ZX_HASHTABLE_LOAD_FACTOR_ = 80;

static inline ZXStatus ZXHashtable_init(ZXAlloc *alloc, zx_u32 capacity,
                                        zx_usize entry_align, zx_u32 entry_size,
                                        ZXHashtable *table,
                                        ZXErrStack *err_stack) {
  zx_u32 cap = zx_next_pow_two_u32(
      (capacity * (zx_u32){100wbu} / ZX_HASHTABLE_LOAD_FACTOR_));
  cap = zx_int_max_u32(ZX_HASHTABLE_GROUP_SIZE_, cap);
  ZX_ASSERT((cap >= capacity));
  ZX_ASSERT((cap > 0));
  // check it is a power of two
  ZX_ASSERT((ZX_IS_POWER_OF_TWO(cap)));
  zx_u32 const cap_mask = cap - 1;

  // align to cachline because why not?
  constexpr zx_usize ALIGN = 64;

  // allocate extra sizeof(simd) bytes so we can do simd loads on any byte index
  // also have to mirror the first group with the extra group at the end to have
  // simple read path.
  zx_usize const metadata_size = cap + ZX_HASHTABLE_GROUP_SIZE_;
  zx_u8 *metadata = nullptr;
  ZX_ERR_TRY((ZXAlloc_aligned_alloc(alloc, ALIGN, metadata_size,
                                    (void **)&metadata, err_stack)),
             err_stack);
  __builtin_memset(metadata, ZX_HASHTABLE_TAG_EMPTY, metadata_size);

  zx_usize const entries_size = (zx_usize){entry_size} * (zx_usize){cap};
  zx_usize const entries_align = ZX_MAX(entry_align, ALIGN);
  void *entries = nullptr;
  ZX_ERR_TRY((ZXAlloc_aligned_alloc(alloc, entries_align, entries_size,
                                    &entries, err_stack)),
             err_stack);

  *table = (ZXHashtable){
      .entries = entries,
      .metadata = metadata,
      .cap_mask = cap_mask,
      .capacity_left = capacity,
  };

  return ZXStatus_Ok;
}

static inline zx_u16 ZXHashtable_remove_lowest_set_bit_(zx_u16 bitmask) {
  return bitmask & (bitmask - 1);
}

static inline zx_u32 ZXHashtable_maybe_mirror_index_(zx_u32 cap_mask,
                                                     zx_u32 index) {
  return ((index - ZX_HASHTABLE_GROUP_SIZE_) & cap_mask) +
         ZX_HASHTABLE_GROUP_SIZE_;
}

// function for checking if keys of two entries are equal
typedef bool zx_hashtable_key_eqfn(void const *ctx, void const *entry_a,
                                   void const *entry_b);

static inline void ZXHashtable_load_(void const *restrict group_ptr, zx_u8 tag,
                                     zx_u16 *restrict tag_match,
                                     zx_u16 *restrict empty_match) {
  typedef unsigned regt;
  static_assert(4 == sizeof(regt));

  regt const tg = (regt){tag};
  regt tm;
  regt em;

  __asm__ volatile("vmovdqu (%2), %%xmm0\n\t"            // load group
                   "vmovd %3, %%xmm1\n\t"                // load tag
                   "vpbroadcastb %%xmm1, %%xmm1\n\t"     // broadcast tag
                   "vpcmpeqb %%xmm0, %%xmm1, %%xmm2\n\t" // tag match
                   "vpmovmskb %%xmm2, %0\n\t"
                   "vpmovmskb %%xmm0, %1\n\t" // empty match
                   : "=r"(tm), "=r"(em)
                   : "m"(group_ptr), "r"(tg)
                   : "xmm0", "xmm1", "xmm2");

  *tag_match = (zx_u16)tm;
  *empty_match = (zx_u16)em;
}

typedef struct ZXHashtableFindIdx {
  zx_u32 idx;
  bool occupied;
} ZXHashtableFindIdx;

static inline ZXHashtableFindIdx
ZXHashtable_find_idx_(ZXHashtable const *const table, zx_u64 const hash,
                      void const *const entry, zx_u32 const entry_size,
                      void const *const ctx, zx_hashtable_key_eqfn key_eqfn) {
  zx_u32 const cap_mask = table->cap_mask;
  zx_u32 stride = 0;

  zx_u8 const tag = (zx_u8)(hash >> 57);
  zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

  zx_u8 const *const meta = table->metadata;
  while (true) {
    zx_u16 tag_match;
    zx_u16 empty_match;
    ZXHashtable_load_(&meta[meta_idx], tag, &tag_match, &empty_match);

    while (tag_match != 0) {
      zx_u8 const tag_pos = zx_ctz_u16(tag_match);

      zx_u8 const *const entries = (zx_u8 const *)table->entries;

      zx_u32 const idx = (meta_idx + tag_pos) & cap_mask;

      void const *const entry_ptr = &entries[entry_size * idx];
      if (key_eqfn(ctx, entry_ptr, entry)) {
        return (ZXHashtableFindIdx){
            .idx = idx,
            .occupied = true,
        };
      } else {
        tag_match = ZXHashtable_remove_lowest_set_bit_(tag_match);
      }
    }

    if (empty_match != 0) {
      zx_u8 const tag_pos = zx_ctz_u16(empty_match);
      return (ZXHashtableFindIdx){
          .idx = (meta_idx + tag_pos) & cap_mask,
          .occupied = false,
      };
    }

    // Should have found an empty slot by now
    ZX_ASSERT((stride <= cap_mask));

    stride += ZX_HASHTABLE_GROUP_SIZE_;
    meta_idx = (meta_idx + stride) & cap_mask;
  }
}

// Returns true if a matching entry existed before and was overwritten.
// Returns false otherwise.
static inline bool
ZXHashtable_insert(ZXHashtable *const table, zx_u64 const hash,
                   void const *const entry, zx_u32 const entry_size,
                   void const *const ctx, zx_hashtable_key_eqfn key_eqfn) {
  zx_u8 const tag = (zx_u8)(hash >> 57);
  ZXHashtableFindIdx const find =
      ZXHashtable_find_idx_(table, hash, entry, entry_size, ctx, key_eqfn);

  zx_u32 const cap_mask = table->cap_mask;
  zx_u32 const idx = find.idx & cap_mask;
  zx_u32 const maybe_mirror_idx =
      ZXHashtable_maybe_mirror_index_(cap_mask, find.idx);

  zx_u8 *const meta = table->metadata;
  zx_u8 *const entries = (zx_u8 *)table->entries;
  void *const entry_ptr = &entries[entry_size * idx];

  meta[idx] = tag;
  meta[maybe_mirror_idx] = tag;
  __builtin_memcpy(entry_ptr, entry, entry_size);

  if (find.occupied) {
    return true;
  } else {
    ZX_ASSERT((table->capacity_left > 0));
    table->capacity_left -= 1;
    return false;
  }
}

// Returns a pointer to the pre-existing entry if a matching entry is found.
// Inserts the new entry and returns a pointer to that if a matching entry isn't
// found.
//
// In any case the returned pointer will point into the table allocation and not
// the provided entry pointer.
static inline void const *
ZXHashtable_get_or_insert(ZXHashtable *const table, zx_u64 const hash,
                          void const *const entry, zx_u32 const entry_size,
                          void const *const ctx,
                          zx_hashtable_key_eqfn key_eqfn) {
  zx_u8 const tag = (zx_u8)(hash >> 57);
  ZXHashtableFindIdx const find =
      ZXHashtable_find_idx_(table, hash, entry, entry_size, ctx, key_eqfn);

  zx_u32 const cap_mask = table->cap_mask;
  zx_u32 const idx = find.idx & cap_mask;
  zx_u32 const maybe_mirror_idx =
      ZXHashtable_maybe_mirror_index_(cap_mask, find.idx);

  zx_u8 *const meta = table->metadata;
  zx_u8 *const entries = (zx_u8 *)table->entries;
  void *const entry_ptr = &entries[entry_size * idx];

  if (find.occupied) {
    return entry_ptr;
  } else {
    ZX_ASSERT((table->capacity_left > 0));
    table->capacity_left -= 1;

    meta[idx] = tag;
    meta[maybe_mirror_idx] = tag;
    __builtin_memcpy(entry_ptr, entry, entry_size);

    return entry_ptr;
  }
}

// returns a pointer to the entry if it is found in the table.
// returns nullptr if a matching entry isn't found.
static inline void const *
ZXHashtable_get(ZXHashtable const *const table, zx_u64 const hash,
                void const *const entry, zx_u32 const entry_size,
                void const *const ctx, zx_hashtable_key_eqfn key_eqfn) {
  ZXHashtableFindIdx const find =
      ZXHashtable_find_idx_(table, hash, entry, entry_size, ctx, key_eqfn);

  if (find.occupied) {
    zx_u32 const cap_mask = table->cap_mask;
    zx_u32 const idx = find.idx & cap_mask;
    zx_u8 const *const entries = (zx_u8 const *)table->entries;
    void const *const entry_ptr = &entries[entry_size * idx];
    return entry_ptr;
  } else {
    return nullptr;
  }
}

#endif

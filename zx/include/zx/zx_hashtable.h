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
#include <zx/zx_branch.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

typedef struct ZXHashtable {
  zx_u8 *entries;
  zx_u8 *metadata;
  zx_u32 cap_mask;
  zx_u32 capacity_left;
  zx_u32 capacity;
  zx_u8 padding[4];
} ZXHashtable;
static_assert(sizeof(ZXHashtable) == sizeof(void *) * 4);

static inline zx_u32 ZXHashtable_capacity_left(ZXHashtable const *table) {
  return table->capacity_left;
}

// size of xmm simd register
constexpr zx_u8 ZX_HASHTABLE_GROUP_SIZE_ = 16;

constexpr zx_u8 ZX_HASHTABLE_TAG_EMPTY = 0b11111111wbu;

constexpr zx_u32 ZX_HASHTABLE_LOAD_FACTOR_ = 80;

static inline ZXStatus ZXHashtable_init(ZXAlloc *alloc, zx_u32 capacity,
                                        zx_u16 entry_align, zx_u32 entry_size,
                                        ZXHashtable *table,
                                        ZXErrStack *err_stack) {
  ZX_ASSERT((alloc != nullptr));
  ZX_ASSERT((table != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  zx_u32 cap = zx_next_pow_two_u32(capacity * (zx_u32){100wbu} /
                                   ZX_HASHTABLE_LOAD_FACTOR_);
  cap = zx_int_max_u32(ZX_HASHTABLE_GROUP_SIZE_, cap);
  ZX_ASSERT((cap > capacity));
  ZX_ASSERT((cap > 0));
  // check it is a power of two
  ZX_ASSERT((zx_is_pow_two_u32(cap)));
  zx_u32 const cap_mask = zx_chk_sub_u32(cap, 1);

  // allocate extra sizeof(simd) bytes so we can do simd loads on any byte index
  // also have to mirror the first group with the extra group at the end to have
  // simple read path.
  zx_u32 const metadata_size = zx_chk_add_u32(cap, ZX_HASHTABLE_GROUP_SIZE_);
  zx_u8 *metadata = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u8), metadata_size, &metadata,
                            err_stack)),
             err_stack);
  __builtin_memset(metadata, ZX_HASHTABLE_TAG_EMPTY, metadata_size);

  zx_u64 const entries_size = zx_chk_mul_u64(entry_size, cap);
  zx_u8 *entries = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, entry_align, entries_size, &entries, err_stack)),
      err_stack);

  *table = (ZXHashtable){
      .entries = entries,
      .metadata = metadata,
      .cap_mask = cap_mask,
      .capacity_left = capacity,
      .capacity = capacity,
  };

  return ZXStatus_Ok;
}

static inline void ZXHashtable_reset(ZXHashtable *table) {
  ZX_ASSERT((table != nullptr));
  zx_u32 const cap = zx_chk_add_u32(table->cap_mask, 1);
  zx_u32 const metadata_size = zx_chk_add_u32(cap, ZX_HASHTABLE_GROUP_SIZE_);
  __builtin_memset(table->metadata, ZX_HASHTABLE_TAG_EMPTY, metadata_size);
  table->capacity_left = table->capacity;
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

static inline void ZXHashtable_load_(void const *group_ptr, zx_u8 tag,
                                     zx_u16 *tag_match, zx_u16 *empty_match) {
  typedef unsigned long regt;
  static_assert(sizeof(regt) == sizeof(void *));

  regt tag_m;
  regt empty_m;

  __asm__("vmovdqu %2, %%xmm0\n\t"              // load group
          "vmovd %3, %%xmm1\n\t"                // load tag
          "vpbroadcastb %%xmm1, %%xmm1\n\t"     // broadcast tag
          "vpcmpeqb %%xmm0, %%xmm1, %%xmm1\n\t" // tag match
          "vpmovmskb %%xmm1, %0\n\t"
          "vpmovmskb %%xmm0, %1\n\t" // empty match
          : "=r"(tag_m), "=r"(empty_m)
          : "m"(*(zx_u8 const(*)[ZX_HASHTABLE_GROUP_SIZE_])group_ptr),
            "r"((regt)tag)
          : "xmm0", "xmm1");

  *tag_match = (zx_u16)tag_m;
  *empty_match = (zx_u16)empty_m;
}

typedef struct ZXHashtableFindIdx {
  zx_u32 idx;
  bool occupied;
} ZXHashtableFindIdx;

static inline ZXHashtableFindIdx
ZXHashtable_find_idx_(ZXHashtable const *const table, zx_u8 const tag,
                      zx_u64 const hash, void const *const entry,
                      zx_u32 const entry_size, void const *const ctx,
                      zx_hashtable_key_eqfn key_eqfn) {
  typedef unsigned char v16qi __attribute__((vector_size(16)));

  zx_u32 const cap_mask = table->cap_mask;
  zx_u32 stride = 0;

  zx_u32 meta_idx = cap_mask & ((zx_u32)(hash));

  zx_u8 const *const meta = table->metadata;
  while (true) {
    v16qi const tag_vec = {
        tag, tag, tag, tag, tag, tag, tag, tag,
        tag, tag, tag, tag, tag, tag, tag, tag,
    };

    v16qi group;
    __builtin_memcpy(&group, &meta[meta_idx], sizeof(group));

    zx_u16 tag_match = (zx_u16)__builtin_ia32_pmovmskb128(tag_vec == group);

    while (tag_match != 0) {
      zx_u8 const tag_pos = zx_ctz_u16(tag_match);

      zx_u8 const *const entries = table->entries;

      zx_u32 const idx = (meta_idx + tag_pos) & cap_mask;

      void const *const entry_ptr = &entries[entry_size * idx];
      if (zx_likely(key_eqfn(ctx, entry_ptr, entry))) {
        ZX_FUZZ_MARKER;

        return (ZXHashtableFindIdx){
            .idx = idx,
            .occupied = true,
        };
      } else {
        ZX_FUZZ_MARKER;

        tag_match = ZXHashtable_remove_lowest_set_bit_(tag_match);
      }
    }

    zx_u16 const empty_match = (zx_u16)__builtin_ia32_pmovmskb128(group);
    if (zx_likely(empty_match != 0)) {
      ZX_FUZZ_MARKER;

      zx_u8 const tag_pos = zx_ctz_u16(empty_match);
      return (ZXHashtableFindIdx){
          .idx = (meta_idx + tag_pos) & cap_mask,
          .occupied = false,
      };
    }

    // Should have found an empty slot by now
    ZX_DBG_ASSERT((stride <= cap_mask));

    stride += ZX_HASHTABLE_GROUP_SIZE_;
    meta_idx = (meta_idx + stride) & cap_mask;

    ZX_FUZZ_MARKER;
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
      ZXHashtable_find_idx_(table, tag, hash, entry, entry_size, ctx, key_eqfn);

  zx_u8 *const entries = table->entries;
  void *const entry_ptr = &entries[entry_size * find.idx];

  __builtin_memcpy(entry_ptr, entry, entry_size);

  if (find.occupied) {
    ZX_FUZZ_MARKER;
    return true;
  } else {
    ZX_ASSERT((table->capacity_left > 0));
    table->capacity_left -= 1;

    zx_u8 *const meta = table->metadata;
    zx_u32 const maybe_mirror_idx =
        ZXHashtable_maybe_mirror_index_(table->cap_mask, find.idx);
    meta[find.idx] = tag;
    meta[maybe_mirror_idx] = tag;

    ZX_FUZZ_MARKER;
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
      ZXHashtable_find_idx_(table, tag, hash, entry, entry_size, ctx, key_eqfn);

  zx_u8 *const entries = table->entries;
  void *const entry_ptr = &entries[entry_size * find.idx];

  if (find.occupied) {
    ZX_FUZZ_MARKER;
    return entry_ptr;
  } else {
    ZX_ASSERT((table->capacity_left > 0));
    table->capacity_left -= 1;

    __builtin_memcpy(entry_ptr, entry, entry_size);

    zx_u8 *const meta = table->metadata;
    zx_u32 const maybe_mirror_idx =
        ZXHashtable_maybe_mirror_index_(table->cap_mask, find.idx);
    meta[find.idx] = tag;
    meta[maybe_mirror_idx] = tag;

    ZX_FUZZ_MARKER;
    return entry_ptr;
  }
}

// returns a pointer to the entry if it is found in the table.
// returns nullptr if a matching entry isn't found.
static inline void const *
ZXHashtable_get(ZXHashtable const *const table, zx_u64 const hash,
                void const *const entry, zx_u32 const entry_size,
                void const *const ctx, zx_hashtable_key_eqfn key_eqfn) {
  zx_u8 const tag = (zx_u8)(hash >> 57);
  ZXHashtableFindIdx const find =
      ZXHashtable_find_idx_(table, tag, hash, entry, entry_size, ctx, key_eqfn);

  if (find.occupied) {
    zx_u8 const *const entries = table->entries;
    void const *const entry_ptr = &entries[entry_size * find.idx];
    ZX_FUZZ_MARKER;
    return entry_ptr;
  } else {
    ZX_FUZZ_MARKER;
    return nullptr;
  }
}

#endif

#ifndef ZX_SLAB_H
#define ZX_SLAB_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

typedef struct ZXSlab {
  bool *is_occupied;
  zx_u32 *next_free;
  zx_u8 *entries;
  zx_u32 first_free_entry;
  zx_u32 n_occupied;
  zx_u32 capacity;
  zx_u32 padding_;
} ZXSlab;
static_assert(sizeof(ZXSlab) == sizeof(void *) * 5);

static inline zx_u32 ZXSlab_n_occupied(ZXSlab const *slab) {
  return slab->n_occupied;
}

static inline zx_u32 ZXSlab_capacity_left(ZXSlab const *slab) {
  return zx_chk_sub_u32(slab->capacity, slab->n_occupied);
}

static inline ZXStatus ZXSlab_init(ZXAlloc *alloc, zx_u32 capacity,
                                   zx_u16 entry_align, zx_u32 entry_size,
                                   ZXSlab *slab, ZXErrStack *err_stack) {
  ZX_ASSERT((alloc != nullptr));
  ZX_ASSERT((slab != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  bool *is_occupied = nullptr;
  zx_u64 const is_occupied_byte_size = zx_chk_mul_u64(capacity, sizeof(bool));
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(bool), is_occupied_byte_size,
                            (zx_u8 **)&is_occupied, err_stack)),
             err_stack);
  __builtin_memset(is_occupied, 0, is_occupied_byte_size);

  zx_u32 *next_free = nullptr;
  zx_u64 const next_free_byte_size = zx_chk_mul_u64(capacity, sizeof(zx_u32));
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u32), next_free_byte_size,
                            (zx_u8 **)&next_free, err_stack)),
             err_stack);
  for (zx_u32 i = 0; i < capacity; i++) {
    next_free[i] = i + 1;
  }

  zx_u8 *entries = nullptr;
  zx_u64 const entries_byte_size = zx_chk_mul_u64(capacity, entry_size);
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, entry_align, entries_byte_size,
                            (zx_u8 **)&entries, err_stack)),
             err_stack);

  *slab = (ZXSlab){
      .is_occupied = is_occupied,
      .next_free = next_free,
      .entries = entries,
      .first_free_entry = 0,
      .n_occupied = 0,
      .capacity = capacity,
  };

  return ZXStatus_Ok;
}

// It is illegal to call this function when `ZXSlab_capacity_left() == 0`.
static inline zx_u32 ZXSlab_insert(ZXSlab *slab, void const *entry,
                                   zx_u32 entry_size) {
  zx_u32 const key_idx = slab->first_free_entry;
  ZX_ASSERT((key_idx != slab->capacity));

  ZX_ASSERT((slab->is_occupied[key_idx] == false));
  slab->is_occupied[key_idx] = true;

  slab->first_free_entry = slab->next_free[key_idx];

  zx_u64 const entry_offset = zx_chk_mul_u64(entry_size, key_idx);
  __builtin_memcpy(&slab->entries[entry_offset], entry, entry_size);

  slab->n_occupied = zx_chk_add_u32(slab->n_occupied, 1);

  return key_idx;
}

// It is invalid to call this function with a key that doesn't exist in the slab
static inline void ZXSlab_remove(ZXSlab *slab, zx_u32 key, zx_u32 entry_size,
                                 void *entry_out) {
  zx_u32 const key_idx = key;

  ZX_ASSERT((slab->is_occupied[key_idx] == true));
  slab->is_occupied[key_idx] = false;

  slab->next_free[key_idx] = slab->first_free_entry;
  slab->first_free_entry = key_idx;

  slab->n_occupied = zx_chk_sub_u32(slab->n_occupied, 1);

  zx_u64 const entry_offset = zx_chk_mul_u64(entry_size, key_idx);
  __builtin_memcpy(entry_out, &slab->entries[entry_offset], entry_size);
}

// It is invalid to call this function with a key that doesn't exist in the slab
static inline void *ZXSlab_get_mut(ZXSlab const *slab, zx_u32 key,
                                   zx_u32 entry_size) {
  zx_u32 const key_idx = key;

  ZX_ASSERT((slab->is_occupied[key_idx] == true));

  zx_u64 const entry_offset = zx_chk_mul_u64(entry_size, key_idx);
  return &slab->entries[entry_offset];
}

// It is invalid to call this function with a key that doesn't exist in the slab
static inline void const *ZXSlab_get(ZXSlab const *slab, zx_u32 key,
                                     zx_u32 entry_size) {
  return ZXSlab_get_mut(slab, key, entry_size);
}

#endif

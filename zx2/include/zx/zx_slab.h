#ifndef ZX_SLAB_H
#define ZX_SLAB_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

template <typename T> struct ZXSlab {
  bool *is_occupied;
  zx_u32 *next_free;
  T *entries;
  zx_u32 first_free_entry;
  zx_u32 n_occupied_;
  zx_u32 capacity;

  static inline ZXStatus init(ZXAlloc alloc, zx_u32 capacity, ZXSlab *slab,
                              ZXErrStack *err_stack) {
    ZX_ASSERT((alloc != nullptr));
    ZX_ASSERT((slab != nullptr));
    ZX_ASSERT((err_stack != nullptr));

    bool *is_occupied = nullptr;
    ZX_ERR_TRY((alloc.alloc<bool>(capacity, &is_occupied, err_stack)),
               err_stack);
    __builtin_memset(is_occupied, 0, sizeof(bool) * capacity);

    zx_u32 *next_free = nullptr;
    ZX_ERR_TRY((alloc->alloc<zx_u32>(capacity, &next_free, err_stack)),
               err_stack);
    for (zx_u32 i = 0; i < capacity; i++) {
      next_free[i] = i + 1;
    }

    T *entries = nullptr;
    ZX_ERR_TRY((alloc->alloc<T>(capacity, &entries, err_stack)), err_stack);

    *slab = ZXSlab{
        .is_occupied = is_occupied,
        .next_free = next_free,
        .entries = entries,
        .first_free_entry = 0,
        .n_occupied_ = 0,
        .capacity = capacity,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 n_occupied() const {
    return this->n_occupied_;
  }

  __attribute__((always_inline)) inline zx_u32 capacity_left() const {
    return zx_chk_sub_u32(this->capacity, this->n_occupied_);
  }

  // It is illegal to call this function when `ZXSlab_capacity_left() == 0`.
  inline zx_u32 insert(T const *entry) {
    zx_u32 const key_idx = this->first_free_entry;
    ZX_ASSERT((key_idx != this->capacity));

    ZX_ASSERT((this->is_occupied[key_idx] == false));
    this->is_occupied[key_idx] = true;

    this->first_free_entry = this->next_free[key_idx];
    this->entries[key_idx] = *entry;
    this->n_occupied_ = zx_chk_add_u32(this->n_occupied_, 1);

    return key_idx;
  }

  // It is invalid to call this function with a key that doesn't exist in the
  // slab
  inline T remove(zx_u32 key) {
    zx_u32 const key_idx = key;

    ZX_ASSERT((this->is_occupied[key_idx] == true));
    this->is_occupied[key_idx] = false;

    this->next_free[key_idx] = this->first_free_entry;
    this->first_free_entry = key_idx;

    this->n_occupied_ = zx_chk_sub_u32(this->n_occupied_, 1);

    return this->entries[key_idx];
  }

  // It is invalid to call this function with a key that doesn't exist in the
  // slab
  inline T *get_mut(zx_u32 key) const {
    zx_u32 const key_idx = key;
    ZX_ASSERT((this->is_occupied[key_idx] == true));
    return &this->entries[key_idx];
  }

  // It is invalid to call this function with a key that doesn't exist in the
  // slab
  inline T const *get(zx_u32 key) const { return this->get_mut(key); }
};

#endif

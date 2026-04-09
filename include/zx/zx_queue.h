#ifndef ZX_QUEUE_H
#define ZX_QUEUE_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

template <typename T> struct ZXQueue {
  T *entries;
  zx_u32 start;
  zx_u32 len_;
  zx_u32 cap_mask;
  zx_u32 capacity;

  static inline ZXStatus init(ZXAlloc alloc, zx_u32 capacity, ZXQueue *queue,
                              ZXErrStack *err_stack) {
    ZX_ASSERT((alloc != nullptr));
    ZX_ASSERT((queue != nullptr));
    ZX_ASSERT((err_stack != nullptr));

    zx_u32 const cap = zx_next_pow_two_u32(capacity);

    T *entries = nullptr;
    ZX_ERR_TRY((alloc.alloc<T>(capacity, &entries, err_stack)), err_stack);

    *queue = (ZXQueue){
        .entries = entries,
        .start = 0,
        .len_ = 0,
        .cap_mask = cap - 1,
        .capacity = capacity,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 capacity_left() const {
    return zx_chk_sub_u32(this->capacity, this->len_);
  }

  __attribute__((always_inline)) inline zx_u32 len() const {
    return this->len_;
  }

  // returns true if push succeeded, false if not.
  // only failure reason is queue being full.
  inline bool push(T const *entry) {
    if (this->capacity == this->len_) {
      ZX_FUZZ_MARKER;
      return false;
    }
    ZX_FUZZ_MARKER;

    zx_u32 const end = (this->start + this->len_) & this->cap_mask;

    this->entries[end] = *entry;
    this->len_++;
    return true;
  }

  // Similar to ZXQueue_push.
  // Updates the queue length but leaves the new entry uninitialized.
  // Returns a pointer to the new uninitialized entry.
  // Returns nullptr if queue has no space.
  inline T *push_in_place() {
    if (this->capacity == this->len_) {
      ZX_FUZZ_MARKER;
      return nullptr;
    }
    ZX_FUZZ_MARKER;

    zx_u32 const end = (this->start + this->len_) & this->cap_mask;
    this->len_++;
    return &this->entries[end];
  }

  // returns true if pop succeeded, false if not.
  // only failure reason is queue being empty.
  inline bool pop(T *entry) {
    if (this->len_ == 0) {
      ZX_FUZZ_MARKER;
      return false;
    }
    ZX_FUZZ_MARKER;

    zx_u32 const start_ = this->start;
    *entry = this->entries[start_];

    this->start = (start_ + 1) & this->cap_mask;
    this->len_--;
    return true;
  }
};

#endif

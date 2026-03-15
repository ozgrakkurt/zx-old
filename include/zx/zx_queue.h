#ifndef ZX_QUEUE_H
#define ZX_QUEUE_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

typedef struct ZXQueue {
  zx_u8 *entries;
  zx_u32 start;
  zx_u32 len;
  zx_u32 cap_mask;
  zx_u32 capacity;
} ZXQueue;

static inline ZXStatus ZXQueue_init(ZXAlloc *alloc, zx_u32 capacity,
                                    zx_u16 entry_align, zx_u32 entry_size,
                                    ZXQueue *queue, ZXErrStack *err_stack) {
  ZX_ASSERT((alloc != nullptr));
  ZX_ASSERT((queue != nullptr));
  ZX_ASSERT((err_stack != nullptr));

  zx_u32 const cap = zx_next_pow_two_u32(capacity);

  zx_u8 *entries = nullptr;
  zx_u64 const entries_byte_size = zx_chk_mul_u64(entry_size, cap);
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, entry_align, entries_byte_size, &entries,
                            err_stack)),
             err_stack);

  *queue = (ZXQueue){
      .entries = entries,
      .start = 0,
      .len = 0,
      .cap_mask = cap - 1,
      .capacity = capacity,
  };

  return ZXStatus_Ok;
}

static inline zx_u32 ZXQueue_capacity_left(ZXQueue const *queue) {
  return zx_chk_sub_u32(queue->capacity, queue->len);
}

// returns true if push succeeded, false if not.
// only failure reason is queue being full.
static inline bool ZXQueue_push(ZXQueue *queue, void const *entry,
                                zx_u32 entry_size) {
  if (queue->capacity == queue->len) {
    ZX_FUZZ_MARKER;
    return false;
  }
  ZX_FUZZ_MARKER;

  zx_u32 const end = (queue->start + queue->len) & queue->cap_mask;

  zx_u64 const entry_offset = zx_chk_mul_u64(entry_size, end);
  __builtin_memcpy(&queue->entries[entry_offset], entry, entry_size);
  queue->len++;
  return true;
}

// returns true if pop succeeded, false if not.
// only failure reason is queue being empty.
static inline bool ZXQueue_pop(ZXQueue *queue, zx_u32 entry_size, void *entry) {
  if (queue->len == 0) {
    ZX_FUZZ_MARKER;
    return false;
  }
  ZX_FUZZ_MARKER;

  zx_u32 const start = queue->start;
  zx_u64 const entry_offset = zx_chk_mul_u64(entry_size, start);
  __builtin_memcpy(entry, &queue->entries[entry_offset], entry_size);

  queue->start = (start + 1) & queue->cap_mask;
  queue->len--;
  return true;
}

#endif

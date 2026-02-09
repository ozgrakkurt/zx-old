#ifndef SHIN_ALLOC_H
#define SHIN_ALLOC_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <stddef.h>

static constexpr shin_usize SHIN_ALLOC_MIN_ALIGN = 64;
_Static_assert(SHIN_ALLOC_MIN_ALIGN >= alignof(max_align_t));
_Static_assert(SHIN_ALLOC_MIN_ALIGN >= alignof(shin_u256));

typedef struct ShinAlloc {
  shin_usize const capacity;
  shin_usize offset;
  shin_u8 *const buf;
} ShinAlloc;

static inline ShinAlloc ShinAlloc_init(shin_u8 *buf, shin_usize capacity) {
  SHIN_ASSERT((buf != nullptr));
  return (ShinAlloc){
      .capacity = capacity,
      .offset = 0,
      .buf = buf,
  };
}

static inline ShinStatus ShinAlloc_reset(ShinAlloc *self,
                                         ShinErrStack *err_stack) {
  SHIN_ASSERT((err_stack != nullptr));
  SHIN_ERR_ASSERT((self != nullptr), err_stack);
  self->offset = 0;
  return ShinStatus_Ok;
}

static inline ShinStatus ShinAlloc_aligned_alloc(ShinAlloc *self,
                                                 shin_usize align,
                                                 shin_usize size, void **ptr,
                                                 ShinErrStack *err_stack) {
  SHIN_ASSERT((err_stack != nullptr));
  SHIN_ERR_ASSERT((self != nullptr), err_stack);
  SHIN_ERR_ASSERT((ptr != nullptr), err_stack);
  SHIN_ERR_ASSERT((*ptr == nullptr), err_stack);

  // alignment should be <= 4KB
  SHIN_ERR_ASSERT((align <= (1 << 12)), err_stack);

  // alignment should be a power of two
  SHIN_ERR_ASSERT(((align & (align - 1)) == 0), err_stack);

  shin_usize const alloc_align =
      align > SHIN_ALLOC_MIN_ALIGN ? align : SHIN_ALLOC_MIN_ALIGN;

  ShinAlloc const state = *self;

  shin_usize const addr = (shin_usize)(&state.buf[state.offset]);
  shin_usize const aligned_addr =
      (addr + alloc_align - 1) / alloc_align * alloc_align;
  shin_usize const padding = aligned_addr - addr;

  if (padding + size + state.offset > state.capacity) {
    SHIN_ERR_RETURN((ShinStatus_OutOfMemory), err_stack);
  }

  SHIN_ASSERT((aligned_addr % alloc_align == 0));
  SHIN_ASSERT((aligned_addr % align == 0));

  *ptr = (void *)aligned_addr;
  self->offset += padding + size;

  return ShinStatus_Ok;
}

static inline ShinStatus ShinAlloc_alloc(ShinAlloc *self, shin_usize size,
                                         void **ptr, ShinErrStack *err_stack) {
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(self, SHIN_ALLOC_MIN_ALIGN, size, ptr,
                                        err_stack)),
               err_stack);
  return ShinStatus_Ok;
}

#endif

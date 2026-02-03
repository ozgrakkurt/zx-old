#ifndef SHIN_ALLOC_H
#define SHIN_ALLOC_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShinAlloc {
  shin_usize const capacity;
  shin_usize offset;
  shin_u8* const buf;
} ShinAlloc;

static inline ShinAlloc ShinAlloc_init(shin_u8* buf, shin_usize capacity) {
  return (ShinAlloc){
      .capacity = capacity,
      .offset = 0,
      .buf = buf,
  };
}

static inline void ShinAlloc_reset(ShinAlloc* self) { self->offset = 0; }

static inline ShinStatus ShinAlloc_aligned_alloc(ShinAlloc* self,
                                                 shin_usize align,
                                                 shin_usize size, void** ptr,
                                                 ShinErrStack* err_stack) {
  SHIN_ERR_ASSERT((self != nullptr), err_stack);
  SHIN_ERR_ASSERT((ptr != nullptr), err_stack);

  // alignment should be <= 4KB
  SHIN_ERR_ASSERT((align <= (1 << 12)), err_stack);

  // alignment should be a power of two
  SHIN_ERR_ASSERT(((align & (align - 1)) == 0), err_stack);

  ShinAlloc const state = *self;

  shin_usize const addr = (shin_usize)(&state.buf[state.offset]);
  shin_usize const aligned_addr = (addr + align - 1) / align * align;
  shin_usize const padding = aligned_addr - addr;

  if (padding + size > state.capacity - state.offset) {
    SHIN_ERR_RETURN((ShinStatus_OutOfMemory), err_stack);
  }

  *ptr = (void*)aligned_addr;
  self->offset += padding + size;

  return ShinStatus_Ok;
}

static inline ShinStatus ShinAlloc_alloc(ShinAlloc* self, shin_usize size,
                                         void** ptr, ShinErrStack* err_stack) {
  shin_usize const align = alignof(max_align_t);
  SHIN_ERR_TRY((ShinAlloc_aligned_alloc(self, align, size, ptr, err_stack)),
               err_stack);
  return ShinStatus_Ok;
}

#ifdef __cplusplus
}
#endif

#endif

#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

static constexpr zx_usize ZX_ALLOC_MIN_ALIGN = 64;
static_assert(ZX_ALLOC_MIN_ALIGN >= alignof(zx_u256));

typedef struct ZXAlloc {
  zx_usize const capacity;
  zx_usize offset;
  zx_u8 *const buf;
} ZXAlloc;

static inline ZXAlloc ZXAlloc_init(zx_u8 *buf, zx_usize capacity) {
  ZX_ASSERT((buf != nullptr));
  ZX_ASSERT((capacity > 0));
  return (ZXAlloc){
      .capacity = capacity,
      .offset = 0,
      .buf = buf,
  };
}

static inline void ZXAlloc_reset(ZXAlloc *self) {
  ZX_ASSERT((self != nullptr));
  self->offset = 0;
}

static inline ZXStatus ZXAlloc_aligned_alloc(ZXAlloc *self, zx_usize align,
                                             zx_usize size, void **ptr,
                                             ZXErrStack *err_stack) {
  ZX_ASSERT((err_stack != nullptr));
  ZX_ASSERT((self != nullptr));
  ZX_ASSERT((ptr != nullptr));
  ZX_ASSERT((*ptr == nullptr));
  ZX_ASSERT((size > 0));

  ZX_ASSERT((align <= (1 << 12)));
  ZX_ASSERT((zx_is_pow_two_usize(align)));

  zx_usize const alloc_align = zx_int_max_usize(align, ZX_ALLOC_MIN_ALIGN);

  ZXAlloc const state = *self;

  zx_usize const addr = (zx_usize)(&state.buf[state.offset]);
  zx_usize const aligned_addr = zx_next_multiple_of_usize(addr, alloc_align);
  ZX_ASSERT((aligned_addr % alloc_align == 0));
  ZX_ASSERT((aligned_addr % align == 0));
  zx_usize const padding = aligned_addr - addr;

  zx_usize const alloc_size = zx_chk_add_usize(padding, size);
  zx_usize const new_offset = zx_chk_add_usize(state.offset, alloc_size);

  if (new_offset > state.capacity) {
    ZX_ERR_RETURN((ZXStatus_OutOfMemory), err_stack);
  }

  *ptr = (void *)aligned_addr;
  self->offset = new_offset;

  return ZXStatus_Ok;
}

static inline ZXStatus ZXAlloc_alloc(ZXAlloc *self, zx_usize size, void **ptr,
                                     ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      (ZXAlloc_aligned_alloc(self, ZX_ALLOC_MIN_ALIGN, size, ptr, err_stack)),
      err_stack);
  return ZXStatus_Ok;
}

#endif

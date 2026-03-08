#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

ZXAlloc ZXAlloc_init(zx_u8 *buf, zx_u64 capacity) {
  ZX_ASSERT((buf != nullptr || capacity == 0));
  return (ZXAlloc){
      .capacity = capacity,
      .offset = 0,
      .buf = buf,
  };
}

void ZXAlloc_reset(ZXAlloc *self) {
  ZX_ASSERT((self != nullptr));
  self->offset = 0;
}

ZXStatus ZXAlloc_aligned_alloc(ZXAlloc *self, zx_u64 align, zx_u64 size,
                               void **ptr, ZXErrStack *err_stack) {
  ZX_ASSERT((err_stack != nullptr));
  ZX_ASSERT((self != nullptr));
  ZX_ASSERT((ptr != nullptr));
  ZX_ASSERT((*ptr == nullptr));
  ZX_ASSERT((align <= (1 << 12)));
  ZX_ASSERT((zx_is_pow_two_u64(align)));

  if (size == 0) {
    *ptr = nullptr;
    return ZXStatus_Ok;
  }

  zx_u64 const alloc_align = zx_int_max_u64(align, ZX_ALLOC_MIN_ALIGN);

  ZXAlloc const state = *self;

  zx_u64 const addr = (zx_u64)(&state.buf[state.offset]);
  zx_u64 const aligned_addr = zx_next_multiple_of_u64(addr, alloc_align);
  ZX_ASSERT((aligned_addr % alloc_align == 0));
  ZX_ASSERT((aligned_addr % align == 0));
  zx_u64 const padding = aligned_addr - addr;

  zx_u64 const alloc_size = zx_chk_add_u64(padding, size);
  zx_u64 const new_offset = zx_chk_add_u64(state.offset, alloc_size);

  if (new_offset > state.capacity) {
    ZX_ERR_RETURN((ZXStatus_OutOfMemory), err_stack);
  }

  *ptr = (void *)aligned_addr;
  self->offset = new_offset;

  return ZXStatus_Ok;
}

ZXStatus ZXAlloc_alloc(ZXAlloc *self, zx_u64 size, void **ptr,
                       ZXErrStack *err_stack) {
  ZX_ERR_TRY(
      (ZXAlloc_aligned_alloc(self, ZX_ALLOC_MIN_ALIGN, size, ptr, err_stack)),
      err_stack);
  return ZXStatus_Ok;
}

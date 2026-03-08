#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

static constexpr zx_u8 ALIGN = 64;
static_assert(ALIGN >= alignof(zx_u256));

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

ZXStatus ZXAlloc_alloc(ZXAlloc *self, zx_u64 size, void **ptr,
                       ZXErrStack *err_stack) {
  ZX_ASSERT((err_stack != nullptr));
  ZX_ASSERT((self != nullptr));
  ZX_ASSERT((ptr != nullptr));
  ZX_ASSERT((*ptr == nullptr));

  if (size == 0) {
    *ptr = nullptr;
    return ZXStatus_Ok;
  }

  ZXAlloc const state = *self;

  static_assert(sizeof(void *) <= sizeof(zx_u64));
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

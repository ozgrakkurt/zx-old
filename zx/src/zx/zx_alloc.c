#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

ZXAlloc ZXAlloc_init(zx_u8 *buf, zx_u64 capacity) {
  ZX_ASSERT((buf != nullptr));

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

ZXStatus ZXAlloc_alloc(ZXAlloc *self, zx_u16 align, zx_u64 size, zx_u8 **ptr,
                       ZXErrStack *err_stack) {
  ZX_ASSERT((err_stack != nullptr));
  ZX_ASSERT((self != nullptr));
  ZX_ASSERT((ptr != nullptr));
  ZX_ASSERT((*ptr == nullptr));

  constexpr zx_u16 MAX_ALIGN = 128;
  ZX_ASSERT((align <= MAX_ALIGN));
  ZX_ASSERT((align > 0));
  ZX_ASSERT((zx_is_pow_two_u16(align)));

  ZXAlloc const state = *self;
  static_assert(sizeof(void *) == sizeof(zx_u64));
  zx_u64 const addr = zx_chk_add_u64((zx_u64)state.buf, state.offset);
  zx_u64 const aligned_addr = zx_next_multiple_of_u64(addr, align);
  zx_u64 const padding = zx_chk_sub_u64(aligned_addr, addr);

  zx_u64 const alloc_size = zx_chk_add_u64(padding, size);
  zx_u64 const new_offset = zx_chk_add_u64(state.offset, alloc_size);

  if (new_offset > state.capacity) {
    ZX_ERR_RETURN((ZXStatus_OutOfMemory), err_stack);
  }

  *ptr = (zx_u8 *)aligned_addr;
  self->offset = new_offset;

  return ZXStatus_Ok;
}

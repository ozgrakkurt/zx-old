#ifndef ZX_ALLOC_H
#define ZX_ALLOC_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct ZXAlloc {
  zx_u64 const capacity;
  zx_u64 offset;
  zx_u8 *const buf;
} ZXAlloc;

ZXAlloc ZXAlloc_init(zx_u8 *buf, zx_u64 capacity);

void ZXAlloc_reset(ZXAlloc *self);

ZXStatus ZXAlloc_alloc(ZXAlloc *self, zx_u16 align, zx_u64 size, zx_u8 **ptr,
                       ZXErrStack *err_stack);

#endif

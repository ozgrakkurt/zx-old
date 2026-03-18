#ifndef ZX_SLICEMAP_H
#define ZX_SLICEMAP_H

typedef struct ZXSlicemap {
  zx_u8 *keys;
  zx_u8 *values;
  zx_u32 len;
  zx_u32 cap;
} ZXSlicemap;

static inline ZXStatus ZXSlicemap_init(ZXAlloc *alloc, zx_u32 capacity,
                                       zx_u16 key_align, zx_u32 key_size,
                                       zx_u16 value_align, zx_u32 value_size)

#endif

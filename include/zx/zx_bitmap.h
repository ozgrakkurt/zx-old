#ifndef ZX_BITMAP_H
#define ZX_BITMAP_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

typedef struct ZXBitmap {
  zx_u64 *storage;
  zx_u32 n_bits;
} ZXBitmap;

static inline ZXStatus ZXBitmap_init(ZXAlloc *alloc, zx_u32 n_bits,
                                     ZXBitmap *bitmap, ZXErrStack *err_stack) {
  zx_u32 const bitmap_len = zx_chk_add_u32(n_bits, 63) / 64;
  zx_u32 const byte_size = zx_chk_mul_u32(bitmap_len, sizeof(zx_u64));
  zx_u64 *storage = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u64), byte_size,
                            (zx_u8 **)&storage, err_stack)),
             err_stack);
  __builtin_memset(storage, 0, byte_size);

  *bitmap = (ZXBitmap){
      .storage = storage,
      .n_bits = n_bits,
  };

  return ZXStatus_Ok;
}

static inline bool ZXBitmap_get(ZXBitmap const *bitmap, zx_u32 idx) {
  ZX_DBG_ASSERT(idx < bitmap->n_bits);

  zx_u32 const byte_index = idx / 8;
  zx_u32 const bit_shift = idx % 8;
  zx_u8 const mask = (zx_u8){1} << bit_shift;
  zx_u8 const byte = ((zx_u8 *)bitmap->storage)[byte_index];
  return (byte & mask) != 0;
}

static inline bool ZXBitmap_set(ZXBitmap *bitmap, zx_u32 idx) {
  ZX_DBG_ASSERT(idx < bitmap->n_bits);

  zx_u32 const byte_index = idx / 8;
  zx_u32 const bit_shift = idx % 8;
  zx_u8 const mask = (zx_u8){1} << bit_shift;
  zx_u8 *byte_p = &((zx_u8 *)bitmap->storage)[byte_index];
  zx_u8 const byte = *byte_p;
  bool const ret = (byte & mask) != 0;
  *byte_p = byte | mask;
  return ret;
}

static inline bool ZXBitmap_unset(ZXBitmap *bitmap, zx_u32 idx) {
  ZX_DBG_ASSERT(idx < bitmap->n_bits);

  zx_u32 const byte_index = idx / 8;
  zx_u32 const bit_shift = idx % 8;
  zx_u8 const mask = (zx_u8){1} << bit_shift;
  zx_u8 *byte_p = &((zx_u8 *)bitmap->storage)[byte_index];
  zx_u8 const byte = *byte_p;
  bool const ret = (byte & mask) != 0;
  *byte_p = byte & ~mask;
  return ret;
}

#endif

#ifndef FIO_WAKE_H
#define FIO_WAKE_H

#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct FioWake {
  ZXBitmap bitmaps[2];
  zx_u8 write_bitmap;
  zx_u8 read_bitmap;
} FioWake;

static inline ZXStatus FioWake_init(ZXAlloc *alloc, zx_u32 capacity,
                                    FioWake *wake, ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXBitmap_init(alloc, capacity, &wake->bitmaps[0], err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXBitmap_init(alloc, capacity, &wake->bitmaps[1], err_stack)),
             err_stack);
  wake->write_bitmap = 0;
  wake->read_bitmap = 1;

  return ZXStatus_Ok;
}

static inline void FioWake_set(FioWake *wake, zx_u32 idx) {
  ZXBitmap_set(&wake->bitmaps[wake->write_bitmap], idx);
}

static inline ZXBitmap const *FioWake_cycle(FioWake *wake) {
  zx_u8 const tmp = wake->write_bitmap;
  wake->write_bitmap = wake->read_bitmap;
  wake->read_bitmap = tmp;
  ZXBitmap_reset(&wake->bitmaps[wake->write_bitmap]);
  return &wake->bitmaps[wake->read_bitmap];
}

#endif

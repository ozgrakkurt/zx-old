#ifndef FIO_WAKE_H
#define FIO_WAKE_H

#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct FioWake {
  ZXBitmap bitmaps[2];
  ZXBitmap *next;
  ZXBitmapIter current;
} FioWake;

static inline ZXStatus FioWake_init(ZXAlloc *alloc, zx_u32 capacity,
                                    FioWake *wake, ZXErrStack *err_stack) {
  ZX_ERR_TRY((ZXBitmap_init(alloc, capacity, &wake->bitmaps[0], err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXBitmap_init(alloc, capacity, &wake->bitmaps[1], err_stack)),
             err_stack);

  wake->next = &wake->bitmaps[0];
  wake->current = ZXBitmap_iter(&wake->bitmaps[1]);

  return ZXStatus_Ok;
}

// Sets a bit in the current write bitmap.
static inline void FioWake_set(FioWake *wake, zx_u32 idx) {
  ZXBitmap_set(wake->next, idx);
}

// Resets current read bitmap, switches write to the current read bitmap.
// Converts current write bitmap to current read(iterator).
//
// It is illegal to call _cycle before the current iterator is finished.
//
// Caller is expected to consume the iterator by calling _iter_next while
// calling FioWake_set for the next round of iteration. Then call _cycle and get
// the new iterator.
static inline void FioWake_cycle(FioWake *wake) {
  // Check the current iter is finished.
  ZX_ASSERT((!ZXBitmapIter_next(&wake->current)));

  wake->current = ZXBitmap_iter(wake->next);

  wake->next =
      (wake->next == &wake->bitmaps[0]) ? &wake->bitmaps[1] : &wake->bitmaps[0];
  ZXBitmap_reset(wake->next);
}

static inline bool FioWake_iter_next(FioWake *wake, zx_u32 *id) {
  return ZXBitmapIter_next(&wake->current, id);
}

#endif

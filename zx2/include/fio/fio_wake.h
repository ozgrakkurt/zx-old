#ifndef FIO_WAKE_H
#define FIO_WAKE_H

#include <zx/zx_alloc.h>
#include <zx/zx_bitmap.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>

struct FioWake {
  ZXBitmap next;
  ZXBitmap current;
  ZXBitmapIter current_iter;

  static ZXStatus init(ZXAlloc *alloc, zx_u32 capacity, FioWake *wake,
                       ZXErrStack *err_stack) {
    ZXBitmap next;
    ZXBitmap current;
    ZX_ERR_TRY((ZXBitmap::init(alloc, capacity, &current, err_stack)),
               err_stack);
    ZX_ERR_TRY((ZXBitmap::init(alloc, capacity, &next, err_stack)), err_stack);

    ZXBitmapIter current_iter = current.iter();

    *wake = Wake{
        .next = next,
        .current = current,
        .current_iter = current_iter,
    };

    return ZXStatus::Ok;
  }

  // Sets a bit in the current write bitmap.
  inline void set(zx_u32 idx) { this->next.set(idx); }

  // Resets current read bitmap, switches write to the current read bitmap.
  // Converts current write bitmap to current read(iterator).
  //
  // It is illegal to call _cycle before the current iterator is finished.
  //
  // Caller is expected to consume the iterator by calling _iter_next while
  // calling FioWake_set for the next round of iteration. Then call _cycle and
  // get the new iterator.
  inline void cycle() {
    // Check the current iter is finished.
    zx_u32 idx = 0;
    ZX_ASSERT((!this->current.next(&idx))));

    ZXBitmap tmp = this->current;
    this->current = this->next;
    this->next = tmp;

    this->current_iter = this->current.iter();
    this->next.reset();
  }

  inline bool iter_next(zx_u32 *id) { return this->current_iter.next(id); }
};

#endif

#ifndef ZX_ARENA_ALLOC_H
#define ZX_ARENA_ALLOC_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

struct ZXArenaAlloc {
  zx_u64 const capacity;
  zx_u64 offset;
  zx_u8 *const buf;

  static inline ZXArenaAlloc init(zx_u8 *buf, zx_u64 capacity) {
    ZX_FUZZ_MARKER;

    if (capacity == 0) [[unlikely]] {
      ZX_FUZZ_MARKER;
      ZX_ASSERT((buf == nullptr));

      return ZXArenaAlloc{
          .capacity = 0,
          .offset = 0,
          .buf = nullptr,
      };
    }
    ZX_FUZZ_MARKER;

    ZX_ASSERT((buf != nullptr));

    return ZXArenaAlloc{
        .capacity = capacity,
        .offset = 0,
        .buf = buf,
    };
  }

  inline void reset() {
    ZX_FUZZ_MARKER;
    this->offset = 0;
  }

  inline ZXAlloc make_alloc() {
    ZX_FUZZ_MARKER;
    return ZXAlloc{
        .ctx = (void *)this,
        .alloc_func = allocfn,
    };
  }

  static inline ZXStatus allocfn(void *ctx, zx_u16 align, zx_u64 size,
                                 void **ptr, ZXErrStack *err_stack) {
    ZX_FUZZ_MARKER;

    ZX_ASSERT((ctx != nullptr));
    ZX_ASSERT((err_stack != nullptr));
    ZX_ASSERT((ptr != nullptr));
    ZX_ASSERT((*ptr == nullptr));

    ZXArenaAlloc *self = (ZXArenaAlloc *)ctx;

    if (self->capacity == 0) [[unlikely]] {
      ZX_FUZZ_MARKER;
      ZX_ASSERT((self->buf == nullptr));
      if (size > 0) {
        ZX_FUZZ_MARKER;
        ZX_ERR_RETURN((ZXStatus::OutOfMemory), err_stack);
      }
      ZX_FUZZ_MARKER;
    }
    ZX_FUZZ_MARKER;

    static_assert(sizeof(void *) == sizeof(zx_u64));
    zx_u64 const addr = zx_chk_add_u64((zx_u64)self->buf, self->offset);
    ZX_ASSERT((align > 0));
    zx_u64 const aligned_addr = zx_next_multiple_of_u64(addr, align);
    zx_u64 const padding = zx_chk_sub_u64(aligned_addr, addr);

    zx_u64 const size = zx_chk_mul_u64(len, sizeof(T));
    zx_u64 const alloc_size = zx_chk_add_u64(padding, size);
    zx_u64 const new_offset = zx_chk_add_u64(self->offset, alloc_size);

    if (new_offset > self->capacity) {
      ZX_FUZZ_MARKER;
      ZX_ERR_RETURN((ZXStatus::OutOfMemory), err_stack);
    }
    ZX_FUZZ_MARKER;

    *ptr = (T *)aligned_addr;
    self->offset = new_offset;

    return ZXStatus::Ok;
  }
};

#endif

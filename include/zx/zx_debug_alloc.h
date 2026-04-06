#ifndef ZX_DEBUG_ALLOC_H
#define ZX_DEBUG_ALLOC_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

struct ZXDebugAlloc {
  zx_u64 capacity;
  zx_u64 offset;
  zx_u8 *buf;

  // Allocates two times the capacity bytes of memory from the OS.
  // Uses this memory to implement an arena style allocator with extra
  // checks using mprotect.
  static inline ZXStatus init(zx_u64 capacity, ZXDebugAlloc *dbg_alloc,
                              ZXErrStack *err_stack) {
    ZX_ASSERT((dbg_alloc != nullptr));
    ZX_ASSERT((err_stack != nullptr));

    if (capacity == 0) [[unlikely]] {
      ZX_FUZZ_MARKER;
      *dbg_alloc = ZXDbgAlloc{
          .capacity = 0,
          .offset = 0,
          .buf = nullptr,
      };

      return ZXStatus::Ok;
    }
    ZX_FUZZ_MARKER;

    zx_u64 const alloc_size =
        zx_next_multiple_of_u64(zx_chk_mul_u64(capacity, 2), ZX_PAGE_SIZE);

    zx_u8 *mem = nullptr;
    ZX_ERR_TRY((zx_page_alloc_4kb(alloc_size, &mem, err_stack)), err_stack);

    *dbg_alloc = ZXDebugAlloc{
        .capacity = alloc_size,
        .offset = 0,
        .buf = mem,
    };

    return ZXStatus::Ok;
  }

  // Resets the allocator to it's initial state.
  // Does NOT deallocate memory.
  // Removes mprotect restrictions on all of the initial memory range.
  inline void reset() {
    ZX_FUZZ_MARKER;

    this->offset = 0;

    if (this->capacity > 0) {
      ZX_FUZZ_MARKER;
      ZXErrStack err_stack = ZXErrStack::init();
      ZX_ASSERT((this->buf != nullptr));
      ZX_ERR_TRY_FATAL(
          (zx_sys_mprotect(this->buf, this->capacity,
                           ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE, &err_stack)),
          &err_stack);
    }

    ZX_FUZZ_MARKER;
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

    ZXDebugAlloc *self = (ZXDebugAlloc *)ctx;

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

    ZX_ASSERT((self->buf != nullptr));

    ZX_ASSERT((self->offset % ZX_PAGE_SIZE == 0));
    ZX_ASSERT((((zx_u64)self->buf) % ZX_PAGE_SIZE == 0));

    ZX_ASSERT((align > 0));
    ZX_ASSERT((align <= ZX_PAGE_SIZE));

    zx_u64 const size = zx_chk_mul_u64(len, sizeof(T));
    zx_u64 const ret_mem_size = zx_next_multiple_of_u64(size, ZX_PAGE_SIZE);
    zx_u64 const alloc_size = zx_chk_add_u64(ret_mem_size, ZX_PAGE_SIZE);
    zx_u64 const new_offset = zx_chk_add_u64(self->offset, alloc_size);

    if (new_offset > self->capacity) {
      ZX_FUZZ_MARKER;
      ZX_ERR_RETURN((ZXStatus::OutOfMemory), err_stack);
    }
    ZX_FUZZ_MARKER;

    ZX_ERR_TRY((zx_sys_mprotect(&self->buf[self->offset + ret_mem_size],
                                ZX_PAGE_SIZE, ZX_SYS_PROT_NONE, err_stack)),
               err_stack);

    *ptr = (T *)&self->buf[self->offset];
    self->offset = new_offset;

    return ZXStatus::Ok;
  }
};

#endif

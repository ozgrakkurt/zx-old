#ifndef ZX_DEBUG_ALLOC_H
#define ZX_DEBUG_ALLOC_H

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

constexpr zx_u64 PAGE_SIZE = 1 << 12;

struct ZXDebugAlloc {
  zx_u64 capacity;
  zx_u64 offset;
  zx_u8 *buf;

  static inline ZXStatus init(zx_u64 capacity, ZXDebugAlloc *dbg_alloc,
                              ZXErrStack *err_stack) {
    ZX_ASSERT((dbg_alloc != nullptr));

    zx_u64 const alloc_size =
        zx_next_multiple_of_u64(zx_chk_mul_u64(capacity, 2), PAGE_SIZE);

    zx_u8 *mem = nullptr;
    ZX_ERR_TRY((zx_page_alloc_4kb(alloc_size, &mem, err_stack)), err_stack);

    *dbg_alloc = ZXDebugAlloc{
        .capacity = alloc_size,
        .offset = 0,
        .buf = mem,
    };

    return ZXStatus::Ok;
  }

  inline void reset() {
    zx_u8 *mem = nullptr;
    ZXErrStack err_stack = ZXErrStack::init();
    ZX_ERR_TRY_FATAL(
        (zx_page_alloc_free(this->buf, this->capacity, &err_stack)),
        &err_stack);
    ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(this->capacity, &mem, &err_stack)),
                     &err_stack);
    this->offset = 0;
    this->buf = mem;
  }

  template <typename T>
  inline ZXStatus aligned_alloc(zx_u16 align, zx_u64 len, T **ptr,
                                ZXErrStack *err_stack) {
    ZX_ASSERT((err_stack != nullptr));
    ZX_ASSERT((ptr != nullptr));
    ZX_ASSERT((*ptr == nullptr));

    ZX_ASSERT((this->offset % PAGE_SIZE == 0));
    ZX_ASSERT((((zx_u64)this->buf) % PAGE_SIZE == 0));

    // We effectively align to 4KB anyway but do same checks to keep
    // compatibility with ZXAlloc
    constexpr zx_u16 MAX_ALIGN = 128;
    ZX_ASSERT((align <= MAX_ALIGN));
    ZX_ASSERT((align > 0));
    ZX_ASSERT((zx_is_pow_two_u16(align)));
    ZX_ASSERT((alignof(T) <= align));

    zx_u64 const size = zx_chk_mul_u64(len, sizeof(T));
    zx_u64 const ret_mem_size = zx_next_multiple_of_u64(size, PAGE_SIZE);
    zx_u64 const alloc_size = zx_chk_add_u64(ret_mem_size, PAGE_SIZE);
    zx_u64 const new_offset = zx_chk_add_u64(this->offset, alloc_size);

    if (new_offset > this->capacity) {
      ZX_ERR_RETURN((ZXStatus::OutOfMemory), err_stack);
    }

    ZX_ERR_TRY((zx_sys_mprotect(&this->buf[this->offset + ret_mem_size],
                                PAGE_SIZE, ZX_SYS_PROT_NONE, err_stack)),
               err_stack);

    *ptr = (T *)&this->buf[this->offset];
    this->offset = new_offset;

    return ZXStatus::Ok;
  }

  template <typename T>
  inline ZXStatus alloc(zx_u64 len, T **ptr, ZXErrStack *err_stack) {
    ZX_ERR_TRY((this->aligned_alloc(alignof(T), len, ptr, err_stack)),
               err_stack);
    return ZXStatus::Ok;
  }
};

#endif

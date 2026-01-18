#ifndef BLIB_ALLOC_HPP
#define BLIB_ALLOC_HPP

#include <cstddef>
#include <cstdint>

#include "./blib_err.hpp"

namespace blib {

struct Alloc {
  uint8_t* buf;
  size_t capacity;
  size_t offset;
};

inline Alloc Alloc_init(uint8_t* buf, size_t capacity) {
  return {.buf = buf, .capacity = capacity, .offset = 0};
}

inline void Alloc_reset(Alloc* obj) { obj->offset = 0; }

inline Status Alloc_alloc(Alloc* obj, size_t align, size_t size,
                          uint8_t** ptr) {
  BLIB_ERR_ASSERT(obj != nullptr);
  BLIB_ERR_ASSERT(ptr != nullptr);

  // Check that alignment is at most 4KB.
  BLIB_ERR_ASSERT(align <= (1 << 12));

  // check that alignment is a power of two.
  BLIB_ERR_ASSERT((align & (align - 1)) == 0);

  Alloc const state = *obj;

  size_t const addr = size_t(&state.buf[state.offset]);
  size_t const aligned_addr = (addr + align - 1) / align * align;
  size_t const padding = aligned_addr - addr;

  if (padding + size > state.capacity) {
    BLIB_ERR_TRY(Status::OutOfMemory);
  }

  *ptr = state.buf + state.offset;
  obj->offset += (padding + size);

  return Status::Ok;
}

}  // namespace blib

#endif

#ifndef BLIB_ALLOC_HPP
#define BLIB_ALLOC_HPP

#include <blib_err.hpp>
#include <blib_int.hpp>
#include <cstddef>
#include <cstdint>

namespace blib {

struct Alloc {
  static Alloc init(uint8_t* buf, size_t capacity) {
    Alloc alloc;
    alloc.buf = buf;
    alloc.capacity = capacity;
    alloc.offset = 0;
    return alloc;
  }

  void reset() { this->offset = 0; }

  template <typename T>
  Status alloc(size_t len, T** ptr) {
    return this->alloc_impl(alignof(T), sizeof(T) * len, (uint8_t**)ptr);
  }

  template <typename T>
  Status aligned_alloc(size_t align, size_t len, T** ptr) {
    size_t const align_ = blib::max<size_t>(alignof(T), align);
    return this->alloc_impl(align_, sizeof(T) * len, (uint8_t**)ptr);
  }

 private:
  uint8_t* buf;
  size_t capacity;
  size_t offset;

  Status alloc_impl(size_t align, size_t size, uint8_t** ptr) {
    BLIB_ERR_ASSERT(ptr != nullptr);

    // Check that alignment is at most 4KB.
    BLIB_ERR_ASSERT(align <= (1 << 12));

    // check that alignment is a power of two.
    BLIB_ERR_ASSERT((align & (align - 1)) == 0);

    Alloc const state = *this;

    size_t const addr = size_t(&state.buf[state.offset]);
    size_t const aligned_addr = (addr + align - 1) / align * align;
    size_t const padding = aligned_addr - addr;

    if (padding + size > state.capacity) {
      BLIB_ERR_TRY(Status::OutOfMemory);
    }

    *ptr = state.buf + state.offset;
    this->offset += (padding + size);

    return Status::Ok;
  }
};

}  // namespace blib

#endif

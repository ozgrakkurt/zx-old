#ifndef BLIB_ALLOC_HPP
#define BLIB_ALLOC_HPP

#include <cstddef>
#include <cstdint>

#include "./blib_err.hpp"

namespace blib {

struct Alloc {
  void* ctx;
  Status (*alloc_)(void* ctx, void** ptr, size_t size, uint8_t align);
  void (*free_)(void* ctx, void* ptr, size_t size);

  template <typename T>
  Status alloc(T** ptr, size_t len) const {
    BLIB_ERR_TRY(this.aligned_alloc(ptr, len, alignof(T)));
    return blib::Status::Ok;
  }

  template <typename T>
  Status aligned_alloc(T** ptr, size_t len, uint8_t align) const {
    // Check that we are not creating a misaligned pointer.
    BLIB_ERR_ASSERT(align >= alignof(T));

    // Check that alignment is at most 4KB.
    BLIB_ERR_ASSERT(align <= (1 << 12));

    // check that alignment is a power of two.
    BLIB_ERR_ASSERT((align & (align - 1)) == 0);

    BLIB_ERR_TRY(this->alloc_(this->ctx, (void**)ptr, sizeof(T) * len, align));

    // Check alignment of the returned pointer.
    // The alloc implementation is buggy if this assertion fails.
    BLIB_ERR_ASSERT(size_t(*ptr) % align == 0);

    return blib::Status::Ok;
  }

  template <typename T>
  void free(T* ptr, size_t len) const {
    this->free_(this->ctx, (void**)ptr, sizeof(T) * len);
  }
};

}  // namespace blib

#endif

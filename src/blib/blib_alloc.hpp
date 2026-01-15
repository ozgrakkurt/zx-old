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
    BLIB_ERR_TRY(
        this->alloc_(this->ctx, (void**)ptr, sizeof(T) * len, alignof(T)));
    return blib::Status::Ok;
  }

  template <typename T>
  Status aligned_alloc(T** ptr, size_t len, uint8_t align) const {
    BLIB_ERR_ASSERT(align >= alignof(T));
    BLIB_ERR_ASSERT((align & (align - 1)) == 0);

    BLIB_ERR_TRY(this->alloc_(this->ctx, (void**)ptr, sizeof(T) * len, align));

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

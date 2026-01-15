#ifndef BLIB_ALLOC_HPP
#define BLIB_ALLOC_HPP

#include <cstdint>

#include "./blib_err.hpp"

namespace blib {

struct Alloc {
  void* ctx;
  Status (*alloc)(void* ctx, void** ptr, size_t size, uint8_t align);
  void (*free)(void* ctx, void* ptr, size_t size);

  template <typename T>
  Status alloc(T** ptr, size_t len) {
    BLIB_ERR_TRY(
        this.alloc(this.ctx, (void**)ptr, sizeof(T) * len, alignof(T)));
    return blib::Status::Ok;
  }

  template <typename T>
  Status aligned_alloc(T** ptr, size_t len, uint8_t align) {
    BLIB_ERR_ASSERT(align >= alignof(T));
    BLIB_ERR_ASSERT((align & (align - 1)) == 0);

    BLIB_ERR_TRY(this.alloc(this.ctx, (void**)ptr, sizeof(T) * len, align));

    BLIB_ERR_ASSERT(size_t(*ptr) % align == 0);

    return blib::Status::Ok;
  }

  template <typename T>
  void free(T* ptr, size_t len) {
    BLIB_ERR_TRY(this.free(this.ctx, (void**)ptr, sizeof(T) * len));
  }
};

struct FixedBufAlloc {};

struct ArenaAlloc {};

struct HugePageAlloc {};

}  // namespace blib

#endif

#ifndef BLIB_ALLOC_HPP
#define BLIB_ALLOC_HPP

#include <cstdint>

#include "./blib_err.hpp"

namespace blib {

namespace alloc {}

struct Alloc {
  void* ctx;
  Status (*alloc)(void* ctx, size_t size, uint8_t align);
  void (*free)(void* ctx, void* ptr, size_t size);
}

}  // namespace blib

#endif

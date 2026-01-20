#ifndef BLIB_INT_HPP
#define BLIB_INT_HPP

#include <cstddef>
#include <cstdint>

namespace blib {

typedef _BitInt(128) int128_t_;
typedef _BitInt(256) int256_t_;
typedef unsigned _BitInt(128) uint128_t_;
typedef unsigned _BitInt(256) uint256_t_;

template <typename T>
T max(T left, T right) {
  return left > right ? left : right;
}

template <typename T>
T min(T left, T right) {
  return left < right ? left : right;
}

template <typename T>
T int_from_bytes(uint8_t const* bytes) {
  T val;
  for (size_t i = 0; i < sizeof(T); i++) {
    ((uint8_t*)&val)[i] = bytes[i];
  }
  return val;
}

}  // namespace blib

#endif

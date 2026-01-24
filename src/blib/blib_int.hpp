#ifndef BLIB_INT_HPP
#define BLIB_INT_HPP

#include <blib_err.hpp>
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
  __builtin_memcpy(&val, bytes, sizeof(T));
  return val;
}

template <typename T>
struct is_unsigned {
  static const bool value = T(-1) > T(0);
};

template <typename T>
Status next_power_of_two(T x, T* out) {
  static_assert(is_unsigned<T>::value, "T must be an unsigned integer type");
  size_t constexpr N_BITS = sizeof(T) * 8;

  if (x <= 1) {
    *out = x;
    return Status::Ok;
  };

  size_t shift = N_BITS - __builtin_clzg(T(x - 1));

  BLIB_ERR_ASSERT((shift < N_BITS));

  *out = T(T(1) << shift);

  return Status::Ok;
}

}  // namespace blib

#endif

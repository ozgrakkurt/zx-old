#ifndef BLIB_RAND_HPP
#define BLIB_RAND_HPP

#include <cstdint>

#include "./blib_int.hpp"

namespace blib {

static uint64_t splitmix64(uint64_t* seed) {
  uint64_t const s = (*seed) + 0x9e3779b97f4a7c15;
  *seed = s;

  uint64_t z = s;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;

  return z ^ (z >> 31);
}

template <typename T>
T random(uint64_t* seed);

#define BLIB_IMPL_RANDOM_SIMPLE_INT_(T) \
  template <>                           \
  T random<T>(uint64_t* seed) {         \
    return T(splitmix64(seed));         \
  }
BLIB_IMPL_RANDOM_SIMPLE_INT_(uint8_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(uint16_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(uint32_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(uint64_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(int8_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(int16_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(int32_t);
BLIB_IMPL_RANDOM_SIMPLE_INT_(int64_t);

#define BLIB_IMPL_RANDOM_INT_128(T)                                   \
  template <>                                                         \
  T random<T>(uint64_t* seed) {                                       \
    blib::uint128_t_ const low = blib::uint128_t_(splitmix64(seed));  \
    blib::uint128_t_ const high = blib::uint128_t_(splitmix64(seed)); \
    return T((high << 64) | low);                                     \
  }
BLIB_IMPL_RANDOM_INT_128(uint128_t_);
BLIB_IMPL_RANDOM_INT_128(int128_t_);

#define BLIB_IMPL_RANDOM_INT_256(T)                                \
  template <>                                                      \
  T random<T>(uint64_t* seed) {                                    \
    blib::uint256_t_ const a = blib::uint256_t_(splitmix64(seed)); \
    blib::uint256_t_ const b = blib::uint256_t_(splitmix64(seed)); \
    blib::uint256_t_ const c = blib::uint256_t_(splitmix64(seed)); \
    blib::uint256_t_ const d = blib::uint256_t_(splitmix64(seed)); \
    return T((a << 192) | (b << 128) | (c << 64) | d);             \
  }
BLIB_IMPL_RANDOM_INT_256(uint256_t_);
BLIB_IMPL_RANDOM_INT_256(int256_t_);

}  // namespace blib

#endif

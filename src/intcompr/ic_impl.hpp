#ifndef INTCOMPR_IC_IMPL_HPP
#define INTCOMPR_IC_IMPL_HPP

#include <cstddef>
#include <cstdint>

#define IC_REPEAT_FOR_IC_TYPES(D) \
  D(uint8_t);                     \
  D(uint16_t);                    \
  D(uint32_t);                    \
  D(uint64_t);                    \
  D(ic_impl::uint128_t_);         \
  D(ic_impl::uint256_t_);         \
  D(int8_t);                      \
  D(int16_t);                     \
  D(int32_t);                     \
  D(int64_t);                     \
  D(ic_impl::int128_t_);          \
  D(ic_impl::int256_t_);

namespace ic_impl {
typedef _BitInt(128) int128_t_;
typedef _BitInt(256) int256_t_;
typedef(unsigned _BitInt(128)) uint128_t_;
typedef(unsigned _BitInt(256)) uint256_t_;
}  // namespace ic_impl

#endif

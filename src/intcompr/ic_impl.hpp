#ifndef INTCOMPR_IC_IMPL_HPP
#define INTCOMPR_IC_IMPL_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

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

#define IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(D) \
  D(uint8_t, uint8_t);                            \
  D(uint16_t, uint16_t);                          \
  D(uint32_t, uint32_t);                          \
  D(uint64_t, uint64_t);                          \
  D(ic_impl::uint128_t_, uint64_t);               \
  D(ic_impl::uint256_t_, uint64_t);               \
  D(int8_t, uint8_t);                             \
  D(int16_t, uint16_t);                           \
  D(int32_t, uint32_t);                           \
  D(int64_t, uint64_t);                           \
  D(ic_impl::int128_t_, uint64_t);                \
  D(ic_impl::int256_t_, uint64_t);

namespace ic_impl {

template <typename T>
inline size_t constexpr n_block_metas_multiplier() {
  if (sizeof(T) == 128) {
    return 2;
  } else if (sizeof(T) == 256) {
    return 4;
  } else {
    return 1;
  }
}

typedef _BitInt(128) int128_t_;
typedef _BitInt(256) int256_t_;
typedef unsigned _BitInt(128) uint128_t_;
typedef unsigned _BitInt(256) uint256_t_;

typedef size_t (*compress_boundfn_t)(size_t n);
typedef size_t (*compress_block_t)(uint8_t* __restrict meta,
                                   void const* __restrict input,
                                   void* __restrict output);

size_t constexpr ALIGNMENT = 64;

template <typename T>
uint8_t needed_bits(T value) {
  if (value == 0) return 0;
  return sizeof(T) * 8 - __builtin_clzll(value);
}

template <typename T>
size_t compress_impl(compress_boundfn_t boundfn,
                     compress_block_t compress_block,
                     size_t meta_size_per_block, void const* __restrict input,
                     size_t input_len, uint8_t* __restrict output,
                     size_t output_size);

}  // namespace ic_impl

#endif

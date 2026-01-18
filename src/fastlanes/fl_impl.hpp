#ifndef FASTLANES_FL_IMPL_HPP
#define FASTLANES_FL_IMPL_HPP

#include <cstddef>
#include <cstdint>

#define FL_REPEAT_FOR_FL_TYPES(D) \
  D(uint8_t);                     \
  D(uint16_t);                    \
  D(uint32_t);                    \
  D(uint64_t);

#if defined(__clang__)
#define FL_UNROLL _Pragma("clang loop unroll_count(1024)")
#elif defined(__GNUC__)
#define FL_UNROLL _Pragma("GCC unroll 1024")
#else
#define FL_UNROLL _Pragma("unroll 1024")
#endif

namespace fl_impl {

const constexpr size_t FL_ORDER[] = {0, 4, 2, 6, 1, 5, 3, 7};

constexpr size_t index(size_t row, size_t lane) {
  size_t const o = row / 8;
  size_t const s = row % 8;
  return (FL_ORDER[o] * 16) + (s * 128) + lane;
}

template <typename T>
T make_mask(size_t width) {
  return T(T(1) << width) - T(1);
}

}  // namespace fl_impl

#endif

#include <cstddef>
#include <cstdlib>

#include "./fl_impl.hpp"

namespace fastlanes {

template <typename T, uint8_t W>
void bit_unpack_fixed(T const* __restrict input, T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  for (size_t lane = 0; lane < N_LANES; lane++) {
    T src = input[lane];
    T tmp = 0;

    FL_UNROLL
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const curr_word = (row * W) / N_BITS;
      size_t const next_word = ((row + 1) * W) / N_BITS;

      size_t const shift = (row * W) % N_BITS;

      if (next_word > curr_word) {
        size_t const remaining_bits = ((row + 1) * W) % N_BITS;
        size_t const current_bits = W - remaining_bits;

        tmp = (src >> shift) & fl_impl::make_mask<T>(current_bits);

        if (next_word < W) {
          src = input[N_LANES * next_word + lane];
          tmp |= (src & fl_impl::make_mask<T>(remaining_bits)) << current_bits;
        }
      } else {
        tmp = (src >> shift) & fl_impl::make_mask<T>(W);
      }

      size_t const idx = fl_impl::index(row, lane);
      output[idx] = tmp;
    }
  }
}

template <typename T>
void bit_unpack_copy(T const* __restrict input, T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  FL_UNROLL
  for (size_t lane = 0; lane < N_LANES; lane++) {
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const idx = fl_impl::index(row, lane);
      output[idx] = input[N_LANES * row + lane];
    }
  }
}

template <typename T>
void bit_unpack_zero(T* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    output[i] = 0;
  }
}

template <typename T, uint8_t WC>
void bit_unpack_dispatch(T const* __restrict input, T* __restrict output,
                         uint8_t width) {
  if (width == WC) {
    bit_unpack_fixed<T, WC>(input, output);
  } else if constexpr (WC < sizeof(T) * 8) {
    bit_unpack_dispatch<T, WC + 1>(input, output, width);
  } else {
    __builtin_unreachable();
  }
}

template <typename T>
void bit_unpack(T const* __restrict input, T* __restrict output,
                uint8_t width) {
  size_t constexpr N_BITS = sizeof(T) * 8;

  if (width == 0) {
    return bit_unpack_zero<T>(output);
  } else if (width == N_BITS) {
    return bit_unpack_copy<T>(input, output);
  } else if (width > N_BITS) {
    __builtin_unreachable();
  } else {
    return bit_unpack_dispatch<T, 1>(input, output, width);
  }
}

#define BIT_UNPACK(T)                                                          \
  template void bit_unpack<T>(T const* __restrict input, T* __restrict output, \
                              uint8_t width);
FL_REPEAT_FOR_FL_TYPES(BIT_UNPACK);

}  // namespace fastlanes

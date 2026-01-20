#include <cassert>
#include <cstddef>
#include <cstdlib>

#include "./fl_impl.hpp"

namespace fastlanes {

template <typename T, uint8_t W>
void for_pack_fixed(T base, T const* __restrict input, T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  T const mask = fl_impl::make_mask<T>(W);

  for (size_t lane = 0; lane < N_LANES; lane++) {
    T tmp = 0;

    FL_UNROLL
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const idx = fl_impl::index(row, lane);
      T const src = (input[idx] - base) & mask;

      if (row == 0) {
        tmp = src;
      } else {
        tmp |= src << (row * W) % N_BITS;
      }

      size_t const curr_word = (row * W) / N_BITS;
      size_t const next_word = ((row + 1) * W) / N_BITS;

      if (next_word > curr_word) {
        output[N_LANES * curr_word + lane] = tmp;
        size_t const remaining_bits = ((row + 1) * W) % N_BITS;
        tmp = src >> (W - remaining_bits);
      }
    }
  }
}

template <typename T>
void for_pack_copy(T base, T const* __restrict input, T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  FL_UNROLL
  for (size_t lane = 0; lane < N_LANES; lane++) {
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const idx = fl_impl::index(row, lane);
      output[N_LANES * row + lane] = input[idx] - base;
    }
  }
}

template <typename T, uint8_t WC>
void for_pack_dispatch(T base, T const* __restrict input, T* __restrict output,
                       uint8_t width) {
  if (width == WC) {
    for_pack_fixed<T, WC>(base, input, output);
  } else if constexpr (WC < sizeof(T) * 8) {
    for_pack_dispatch<T, WC + 1>(base, input, output, width);
  } else {
    assert(false);
  }
}

template <typename T>
void for_pack(T base, T const* __restrict input, T* __restrict output,
              uint8_t width) {
  size_t constexpr N_BITS = sizeof(T) * 8;

  if (width == 0) {
    return;
  } else if (width == N_BITS) {
    return for_pack_copy<T>(base, input, output);
  } else if (width > N_BITS) {
    assert(false);
  } else {
    return for_pack_dispatch<T, 1>(base, input, output, width);
  }
}

#define FOR_PACK(T)                                            \
  template void for_pack<T>(T base, T const* __restrict input, \
                            T* __restrict output, uint8_t width);
FL_REPEAT_FOR_FL_TYPES(FOR_PACK);

}  // namespace fastlanes

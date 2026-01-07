#include <cstddef>

#include "./fl_impl.hpp"

namespace fastlanes {

template <typename T>
void delta_encode(T const* __restrict bases, T const* __restrict input,
                  T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  for (size_t lane = 0; lane < N_LANES; lane++) {
    T prev = bases[lane];

    FL_UNROLL
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const idx = fl_impl::index(row, lane);
      T const next = input[idx];
      output[idx] = next - prev;
      prev = next;
    }
  }
}

template <typename T>
void delta_decode(T const* __restrict bases, T const* __restrict input,
                  T* __restrict output) {
  size_t constexpr N_BITS = sizeof(T) * 8;
  size_t constexpr N_LANES = 1024 / N_BITS;

  for (size_t lane = 0; lane < N_LANES; lane++) {
    T prev = bases[lane];

    FL_UNROLL
    for (size_t row = 0; row < N_BITS; row++) {
      size_t const idx = fl_impl::index(row, lane);
      T const next = input[idx] + prev;
      output[idx] = next;
      prev = next;
    }
  }
}

constexpr size_t transpose_idx(size_t idx) {
  size_t const lane = idx % 16;
  size_t const order = (idx / 16) % 8;
  size_t const row = idx / 128;

  return (lane * 64) + (fl_impl::FL_ORDER[order] * 8) + row;
}

template <typename T>
void transpose(T const* __restrict input, T* __restrict output) {
  for (size_t i = 0; i < 1024; ++i) {
    output[i] = input[transpose_idx(i)];
  }
}

template <typename T>
void untranspose(T const* __restrict input, T* __restrict output) {
  for (size_t i = 0; i < 1024; ++i) {
    output[transpose_idx(i)] = input[i];
  }
}

#define DELTA_ENCODE(T)                                    \
  template void delta_encode<T>(T const* __restrict bases, \
                                T const* __restrict input, \
                                T* __restrict output);
FL_REPEAT_FOR_FL_TYPES(DELTA_ENCODE);

#define DELTA_DECODE(T)                                    \
  template void delta_decode<T>(T const* __restrict bases, \
                                T const* __restrict input, \
                                T* __restrict output);
FL_REPEAT_FOR_FL_TYPES(DELTA_DECODE);

#define TRANSPOSE(T) \
  template void transpose<T>(T const* __restrict input, T* __restrict output);
FL_REPEAT_FOR_FL_TYPES(TRANSPOSE);

#define UNTRANSPOSE(T) \
  template void untranspose<T>(T const* __restrict input, T* __restrict output);
FL_REPEAT_FOR_FL_TYPES(UNTRANSPOSE);

}  // namespace fastlanes

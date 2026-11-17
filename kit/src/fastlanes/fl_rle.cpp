#include <cstddef>
#include <cstdint>
#include <fl_impl.hpp>

namespace fastlanes {

template <typename T>
size_t rle_encode(T const* __restrict input, T* __restrict rle_vals,
                  uint16_t* __restrict rle_idxs) {
  rle_vals[0] = input[0];
  rle_idxs[0] = 0;

  uint16_t rle_val_idx = 0;
  T prev = input[0];
  for (size_t i = 1; i < 1024; i++) {
    T const curr = input[i];
    if (curr != prev) {
      rle_val_idx += 1;
      rle_vals[rle_val_idx] = curr;
      prev = curr;
    }
    rle_idxs[i] = rle_val_idx;
  }

  return rle_val_idx + 1;
}

template <typename T>
void rle_decode(T const* __restrict rle_vals,
                uint16_t const* __restrict rle_idxs, T* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    output[i] = rle_vals[rle_idxs[i]];
  }
}

#define RLE_ENCODE(T)                                      \
  template size_t rle_encode<T>(T const* __restrict input, \
                                T* __restrict rle_vals,    \
                                uint16_t* __restrict rle_idxs);
FL_REPEAT_FOR_FL_TYPES(RLE_ENCODE);

#define RLE_DECODE(T)                                              \
  template void rle_decode<T>(T const* __restrict rle_vals,        \
                              uint16_t const* __restrict rle_idxs, \
                              T* __restrict output);
FL_REPEAT_FOR_FL_TYPES(RLE_DECODE);

}  // namespace fastlanes

#ifndef FASTLANES_HPP
#define FASTLANES_HPP

#include <cstddef>
#include <cstdint>

namespace fastlanes {

template <typename T>
void delta_encode(T const* __restrict bases, T const* __restrict input,
                  T* __restrict output);

template <typename T>
void delta_decode(T const* __restrict bases, T const* __restrict input,
                  T* __restrict output);

template <typename T>
void transpose(T const* __restrict input, T* __restrict output);

template <typename T>
void untranspose(T const* __restrict input, T* __restrict output);

template <typename T>
void bit_pack(T const* __restrict input, T* __restrict output, uint8_t width);

template <typename T>
void bit_unpack(T const* __restrict input, T* __restrict output, uint8_t width);

template <typename T>
void for_pack(T base, T const* __restrict input, T* __restrict output,
              uint8_t width);

template <typename T>
void for_unpack(T base, T const* __restrict input, T* __restrict output,
                uint8_t width);

template <typename T>
void bit_unpack_delta_decode(T const* __restrict bases,
                             T const* __restrict input, T* __restrict output,
                             uint8_t width);

template <typename T>
size_t rle_encode(T const* __restrict input, T* __restrict rle_vals,
                  uint16_t* __restrict rle_idxs);

template <typename T>
void rle_decode(T const* __restrict rle_vals,
                uint16_t const* __restrict rle_idxs, T* __restrict output);

}  // namespace fastlanes

#endif

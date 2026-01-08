#ifndef INTCOMPR_HPP
#define INTCOMPR_HPP

#include <cstddef>
#include <cstdint>

namespace intcompr {

bool is_err(size_t res);

template <typename T>
size_t bit_pack_compress_bound(size_t n);

template <typename T>
size_t bit_pack_compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size);

template <typename T>
size_t bit_pack_decompress(uint8_t const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len);

template <typename T>
size_t for_pack_compress_bound(size_t n);

template <typename T>
size_t for_pack_compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size);

template <typename T>
size_t for_pack_decompress(uint8_t const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len);

template <typename T>
size_t delta_pack_compress_bound(size_t n);

template <typename T>
size_t delta_pack_compress(T const* __restrict input, size_t input_len,
                           uint8_t* __restrict output, size_t output_size);

template <typename T>
size_t delta_pack_decompress(uint8_t const* __restrict input, size_t input_size,
                             T* __restrict output, size_t output_len);

}  // namespace intcompr

#endif

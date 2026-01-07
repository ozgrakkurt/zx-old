#include <climits>
#include <cstddef>
#include <cstdlib>

#include "./ic_impl.hpp"

namespace intcomp {

template <typename T>
size_t bit_pack_compress_bound(size_t n) {
  // return error if n is too big
  if (n >= INT_MAX) {
    return SIZE_MAX;
  }

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  size_t constexpr size_per_block = sizeof(uint8_t) + 1024 * sizeof(T);

  // one byte for the size of the padding and maximum 63 bytes of padding for
  // alignment
  size_t constexpr padding_size = 64;

  return padding_size + size_per_block * n_blocks + n_remainder * sizeof(T);
}

template <typename T>
size_t bit_pack_compress_block(void* __restrict meta, T const* __restrict input,
                               T* __restrict output);

template <typename T>
size_t bit_pack_compress_block_unsigned(void* __restrict meta,
                                        T const* __restrict input,
                                        T* __restrict output) {
  T m = input[0];
  for (size_t i = 0; i < 1024; i++) {
    m = m < input[i] ? input[i] : m;
  }

  uint8_t const width = ic_impl::needed_bits<T>(m);

  meta[0] = width;

  fastlanes::bit_pack(input, output, width);

  return 1024 * width / (sizeof(T) * 8);
}

template <typename T>
size_t bit_pack_compress(T const* __restrict input, size_t input_len,
                         void* __restrict output, size_t output_size) {
  // return error if output size isn't bigger than the bound
  size_t const bound = bit_pack_compress_bound(input_len);
  if (bound == SIZE_MAX || output_size < bound) {
    return SIZE_MAX;
  }
}

template <typename T>
size_t bit_pack_decompress(void const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len) {}

}  // namespace intcomp

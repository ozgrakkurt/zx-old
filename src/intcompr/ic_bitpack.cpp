#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fastlanes.hpp>

#include "./ic_impl.hpp"
#include "./ic_zigzag.hpp"

namespace intcomp {

template <typename T>
size_t bit_pack_compress_bound(size_t n) {
  // return error if n is too big
  if (n >= INT_MAX) {
    return SIZE_MAX;
  }

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  // 1 byte for metadata (width)
  size_t constexpr size_per_block = sizeof(uint8_t) + 1024 * sizeof(T);

  // one byte for the size of the padding and maximum 63 bytes of padding for
  // alignment
  size_t constexpr padding_size = 64;

  return padding_size + size_per_block * n_blocks + n_remainder * sizeof(T);
}

#define COMPR_BOUND(T) template size_t bit_pack_compress_bound<T>(size_t n);
IC_REPEAT_FOR_IC_TYPES(COMPR_BOUND);

template <typename T>
size_t bit_pack_compress_block_unsigned(uint8_t* __restrict meta,
                                        void const* __restrict input,
                                        void* __restrict output) {
  auto in = (T const *__restrict) input;
  auto out = (T *__restrict) output;

  T m = in[0];
  for (size_t i = 0; i < 1024; i++) {
    m = m < in[i] ? in[i] : m;
  }

  uint8_t const width = ic_impl::needed_bits<T>(m);

  meta[0] = width;

  fastlanes::bit_pack<T>(in, out, width);

  return 1024 * width / (sizeof(T) * 8);
}

template <typename T, typename U>
size_t bit_pack_compress_block_signed(uint8_t* __restrict meta,
                                        void const* __restrict input,
                                        void* __restrict output) {
  auto in = (T const *__restrict) input;
  auto out = (T *__restrict) output;

  T m = in[0];
  for (size_t i = 0; i < 1024; i++) {
    m = m < in[i] ? in[i] : m;
  }

  uint8_t const width = ic_impl::needed_bits<T>(m);

  meta[0] = width;

  alignas(64) U zigzagged[1024]; 

  ic_zigzag::zigzag_encode(in, zigzagged, 1024);

  fastlanes::bit_pack<U>(zigzagged, out, width);

  return 1024 * width / (sizeof(T) * 8);
}

template <typename T>
size_t bit_pack_compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size);

template <>
size_t bit_pack_compress<uint8_t>(uint8_t const* __restrict input,
                                  size_t input_len, uint8_t* __restrict output,
                                  size_t output_size) {
  return ic_impl::compress_data<uint8_t>(
      &bit_pack_compress_bound<uint8_t>, &bit_pack_compress_block_unsigned<uint8_t>,
      sizeof(uint8_t), input, input_len, output, output_size);
}

}  // namespace intcomp

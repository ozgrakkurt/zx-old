#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fastlanes.hpp>

#include "./ic_impl.hpp"

namespace intcomp {

size_t constexpr META_PER_BLOCK = sizeof(uint8_t);

template <typename T>
size_t bit_pack_compress_bound(size_t n) {
  // return error if n is too big
  assert(n < INT_MAX);

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  size_t const metadata_size =
      META_PER_BLOCK * n_blocks * ic_impl::n_block_metas_multiplier<T>();

  size_t constexpr max_data_per_block = 1024 * sizeof(T);

  size_t const max_data_size = max_data_per_block * n_blocks;

  // one byte for the size of the padding and maximum 63 bytes of padding for
  // alignment
  size_t constexpr max_padding_size = 64;

  return n_remainder * sizeof(T) + metadata_size + max_padding_size +
         max_data_size;
}

#define COMPR_BOUND(T) template size_t bit_pack_compress_bound<T>(size_t n);
IC_REPEAT_FOR_IC_TYPES(COMPR_BOUND);

template <typename T>
size_t bit_pack_compress_block(uint8_t* __restrict meta,
                               void const* __restrict input,
                               void* __restrict output) {
  auto in = (T const* __restrict)input;
  auto out = (T* __restrict)output;

  T m = in[0];
  for (size_t i = 0; i < 1024; i++) {
    m = m < in[i] ? in[i] : m;
  }

  uint8_t const width = ic_impl::needed_bits<T>(m);

  meta[0] = width;

  fastlanes::bit_pack<T>(in, out, width);

  return 1024 / 8 * width;
}

template <typename T>
size_t bit_pack_compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size);

#define BITPACKCOMPRESS(T, BT)                                             \
  template <>                                                              \
  size_t bit_pack_compress<T>(T const* __restrict input, size_t input_len, \
                              uint8_t* __restrict output,                  \
                              size_t output_size) {                        \
    return ic_impl::compress_impl<T>(                                      \
        &bit_pack_compress_bound<T>, &bit_pack_compress_block<BT>,         \
        META_PER_BLOCK, input, input_len, output, output_size);            \
  }
IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(BITPACKCOMPRESS);

}  // namespace intcomp

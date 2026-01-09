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
  if (n >= INT_MAX) {
    return SIZE_MAX;
  }

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

  return 1024 * width / (sizeof(T) * 8);
}

template <typename T>
size_t bit_pack_compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size);

template <>
size_t bit_pack_compress<uint8_t>(uint8_t const* __restrict input,
                                  size_t input_len, uint8_t* __restrict output,
                                  size_t output_size) {
  return ic_impl::compress_impl<uint8_t>(
      &bit_pack_compress_bound<uint8_t>, &bit_pack_compress_block<uint8_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<uint16_t>(uint16_t const* __restrict input,
                                   size_t input_len, uint8_t* __restrict output,
                                   size_t output_size) {
  return ic_impl::compress_impl<uint16_t>(
      &bit_pack_compress_bound<uint16_t>, &bit_pack_compress_block<uint16_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<uint32_t>(uint32_t const* __restrict input,
                                   size_t input_len, uint8_t* __restrict output,
                                   size_t output_size) {
  return ic_impl::compress_impl<uint32_t>(
      &bit_pack_compress_bound<uint32_t>, &bit_pack_compress_block<uint32_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<uint64_t>(uint64_t const* __restrict input,
                                   size_t input_len, uint8_t* __restrict output,
                                   size_t output_size) {
  return ic_impl::compress_impl<uint64_t>(
      &bit_pack_compress_bound<uint64_t>, &bit_pack_compress_block<uint64_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<ic_impl::uint128_t_>(
    ic_impl::uint128_t_ const* __restrict input, size_t input_len,
    uint8_t* __restrict output, size_t output_size) {
  return ic_impl::compress_impl<ic_impl::uint128_t_>(
      &bit_pack_compress_bound<ic_impl::uint128_t_>,
      &bit_pack_compress_block<uint64_t>, META_PER_BLOCK, input, input_len,
      output, output_size);
}

template <>
size_t bit_pack_compress<ic_impl::uint256_t_>(
    ic_impl::uint256_t_ const* __restrict input, size_t input_len,
    uint8_t* __restrict output, size_t output_size) {
  return ic_impl::compress_impl<ic_impl::uint256_t_>(
      &bit_pack_compress_bound<ic_impl::uint256_t_>,
      &bit_pack_compress_block<uint64_t>, META_PER_BLOCK, input, input_len,
      output, output_size);
}

template <>
size_t bit_pack_compress<int8_t>(int8_t const* __restrict input,
                                 size_t input_len, uint8_t* __restrict output,
                                 size_t output_size) {
  return ic_impl::compress_impl<int8_t>(
      &bit_pack_compress_bound<int8_t>, &bit_pack_compress_block<uint8_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<int16_t>(int16_t const* __restrict input,
                                  size_t input_len, uint8_t* __restrict output,
                                  size_t output_size) {
  return ic_impl::compress_impl<uint16_t>(
      &bit_pack_compress_bound<int16_t>, &bit_pack_compress_block<uint16_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<int32_t>(int32_t const* __restrict input,
                                  size_t input_len, uint8_t* __restrict output,
                                  size_t output_size) {
  return ic_impl::compress_impl<uint32_t>(
      &bit_pack_compress_bound<int32_t>, &bit_pack_compress_block<uint32_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<int64_t>(int64_t const* __restrict input,
                                  size_t input_len, uint8_t* __restrict output,
                                  size_t output_size) {
  return ic_impl::compress_impl<uint64_t>(
      &bit_pack_compress_bound<int64_t>, &bit_pack_compress_block<uint64_t>,
      META_PER_BLOCK, input, input_len, output, output_size);
}

template <>
size_t bit_pack_compress<ic_impl::int128_t_>(
    ic_impl::int128_t_ const* __restrict input, size_t input_len,
    uint8_t* __restrict output, size_t output_size) {
  return ic_impl::compress_impl<ic_impl::int128_t_>(
      &bit_pack_compress_bound<ic_impl::int128_t_>,
      &bit_pack_compress_block<uint64_t>, META_PER_BLOCK, input, input_len,
      output, output_size);
}

template <>
size_t bit_pack_compress<ic_impl::int256_t_>(
    ic_impl::int256_t_ const* __restrict input, size_t input_len,
    uint8_t* __restrict output, size_t output_size) {
  return ic_impl::compress_impl<ic_impl::int256_t_>(
      &bit_pack_compress_bound<ic_impl::int256_t_>,
      &bit_pack_compress_block<uint64_t>, META_PER_BLOCK, input, input_len,
      output, output_size);
}

}  // namespace intcomp

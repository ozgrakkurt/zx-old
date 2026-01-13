#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fastlanes.hpp>

#include "./ic_impl.hpp"
#include "./intcompr.hpp"

namespace intcompr {

size_t constexpr META_PER_BLOCK = sizeof(uint8_t);

// This is fastlanes N_LANES * sizeof(T).
// Calculated as 1024 / (sizeof(T) * 8) * sizeof(T).
size_t constexpr BASES_SIZE = 128;

template <typename T>
size_t delta_pack_compress_bound(size_t n) {
  assert(n < INT_MAX);

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  size_t const metadata_size =
      META_PER_BLOCK * n_blocks * ic_impl::n_block_metas_multiplier<T>();

  size_t constexpr max_data_per_block = 1024 * sizeof(T) + BASES_SIZE;

  size_t const max_data_size = max_data_per_block * n_blocks;

  // one byte for the size of the padding and maximum 63 bytes of padding for
  // alignment
  size_t constexpr max_padding_size = 64;

  size_t const bound = n_remainder * sizeof(T) + metadata_size +
                       max_padding_size + max_data_size;

  assert(bound < SIZE_MAX);

  return bound;
}

#define COMPR_BOUND(T) template size_t delta_pack_compress_bound<T>(size_t n);
IC_REPEAT_FOR_IC_TYPES(COMPR_BOUND);

template <typename T>
static size_t delta_pack_compress_block(uint8_t* __restrict meta,
                                        void const* __restrict input,
                                        void* __restrict output) {
  auto in = (T const* __restrict)input;
  auto out = (T* __restrict)output;

  alignas(64) T transposed[1024];
  fastlanes::transpose<T>(in, transposed);

  alignas(64) T deltas[1024];
  fastlanes::delta_encode<T>(transposed, transposed, deltas);

  T m = deltas[0];
  for (size_t i = 0; i < 1024; i++) {
    T const v = deltas[i];
    m = m < v ? v : m;
  }

  uint8_t const width = ic_impl::needed_bits<T>(m);

  meta[0] = width;

  size_t constexpr N_BASES = BASES_SIZE / sizeof(T);
  memcpy(out, transposed, BASES_SIZE);

  fastlanes::bit_pack<T>(deltas, &out[N_BASES], width);

  return (1024 / 8) * width + BASES_SIZE;
}

template <typename T>
static size_t delta_pack_decompress_block(uint8_t const* __restrict meta,
                                          void const* __restrict input,
                                          void* __restrict output) {
  auto in = (T const* __restrict)input;
  auto out = (T* __restrict)output;
  size_t constexpr N_BASES = BASES_SIZE / sizeof(T);

  uint8_t const width = meta[0];

  alignas(64) T transposed[1024];
  fastlanes::bit_unpack_delta_decode(in, &in[N_BASES], transposed, width);

  fastlanes::untranspose<T>(transposed, out);

  return (1024 / 8) * width + BASES_SIZE;
}

#define DELTAPACK_COMPRESS(T, BT)                                            \
  template <>                                                                \
  size_t delta_pack_compress<T>(T const* __restrict input, size_t input_len, \
                                uint8_t* __restrict output,                  \
                                size_t output_size) {                        \
    assert(input_len < INT_MAX);                                             \
    assert(output_size >= delta_pack_compress_bound<T>(input_len));          \
    (void)output_size;                                                       \
    return ic_impl::compress_impl<T>(&delta_pack_compress_block<BT>,         \
                                     META_PER_BLOCK, input, input_len,       \
                                     output);                                \
  }
IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(DELTAPACK_COMPRESS);

template <typename T, typename BT>
static bool is_decompress_input_valid(uint8_t const* __restrict input,
                                      size_t input_size, size_t output_len) {
  if (output_len == 0) {
    return true;
  }

  if (input_size >= SIZE_MAX || output_len >= INT_MAX) {
    return false;
  }

  size_t const n_block_metas =
      (output_len / 1024) * ic_impl::n_block_metas_multiplier<T>();
  size_t const n_remainder = output_len % 1024;

  size_t const total_meta_size = n_block_metas;

  if (input_size <= total_meta_size) {
    return false;
  }

  size_t block_data_size = 0;

  for (size_t i = 0; i < n_block_metas; i++) {
    uint8_t const width = input[i];

    if (width > sizeof(BT) * 8) {
      return false;
    }

    block_data_size += (1024 / 8) * width + BASES_SIZE;
  }

  uint8_t const padding = input[total_meta_size];
  size_t const expected_size = total_meta_size + 1 + size_t(padding) +
                               block_data_size + n_remainder * sizeof(T);

  if (input_size != expected_size) {
    return false;
  }

  return true;
}

#define DELTAPACK_DECOMPRESS(T, BT)                                         \
  template <>                                                               \
  size_t delta_pack_decompress<T>(uint8_t const* __restrict input,          \
                                  size_t input_size, T* __restrict output,  \
                                  size_t output_len) {                      \
    if (!is_decompress_input_valid<T, BT>(input, input_size, output_len)) { \
      return SIZE_MAX;                                                      \
    }                                                                       \
    return ic_impl::decompress_impl<T>(&delta_pack_decompress_block<BT>,    \
                                       META_PER_BLOCK, input, output,       \
                                       output_len);                         \
  }
IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(DELTAPACK_DECOMPRESS);

}  // namespace intcompr

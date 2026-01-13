#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fastlanes.hpp>

#include "./ic_impl.hpp"
#include "./intcompr.hpp"

namespace intcompr {

template <typename T>
static size_t meta_size_per_block() {
  if (sizeof(T) == 32) {
    return 2 * sizeof(uint64_t);
  } else if (sizeof(T) == 16) {
    return 2 * sizeof(uint64_t);
  } else {
    return 2 * sizeof(T);
  }
}

template <typename T>
size_t for_pack_compress_bound(size_t n) {
  assert(n < INT_MAX);

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  size_t const metadata_size = meta_size_per_block<T>() * n_blocks *
                               ic_impl::n_block_metas_multiplier<T>();

  size_t constexpr max_data_per_block = 1024 * sizeof(T);

  size_t const max_data_size = max_data_per_block * n_blocks;

  // one byte for the size of the padding and maximum 63 bytes of padding for
  // alignment
  size_t constexpr max_padding_size = 64;

  size_t const bound = n_remainder * sizeof(T) + metadata_size +
                       max_padding_size + max_data_size;

  assert(bound < SIZE_MAX);

  return bound;
}

#define COMPR_BOUND(T) template size_t for_pack_compress_bound<T>(size_t n);
IC_REPEAT_FOR_IC_TYPES(COMPR_BOUND);

template <typename T>
static size_t for_pack_compress_block(uint8_t* __restrict meta,
                                      void const* __restrict input,
                                      void* __restrict output) {
  auto in = (T const* __restrict)input;
  auto out = (T* __restrict)output;

  T ma = in[0];
  T mi = in[0];
  for (size_t i = 0; i < 1024; i++) {
    T const v = in[i];
    ma = ma < v ? v : ma;
    mi = mi > v ? v : mi;
  }

  T const base = mi;
  T const range = ma - mi;

  uint8_t const width = ic_impl::needed_bits<T>(range);

  // should be fine because metadata is written to the start of the compressed
  // buffer. and that buffer should always be aligned to 64 bytes. and we always
  // write two T values to it so it stays aligned to sizeof(T).
  assert(size_t(meta) % sizeof(T) == 0);
  auto meta_p = (T*)meta;
  meta_p[0] = T(width);
  meta_p[1] = base;

  fastlanes::for_pack<T>(base, in, out, width);

  return (1024 / 8) * width;
}

template <typename T>
static size_t for_pack_decompress_block(uint8_t const* __restrict meta,
                                        void const* __restrict input,
                                        void* __restrict output) {
  auto in = (T const* __restrict)input;
  auto out = (T* __restrict)output;

  assert(size_t(meta) % sizeof(T) == 0);
  auto meta_p = (T const*)meta;
  uint8_t const width = uint8_t(meta_p[0]);
  T const base = meta_p[1];

  fastlanes::for_unpack<T>(base, in, out, width);

  return (1024 / 8) * width;
}

#define FORPACK_COMPRESS(T, BT)                                            \
  template <>                                                              \
  size_t for_pack_compress<T>(T const* __restrict input, size_t input_len, \
                              uint8_t* __restrict output,                  \
                              size_t output_size) {                        \
    assert(input_len < INT_MAX);                                           \
    assert(output_size >= for_pack_compress_bound<T>(input_len));          \
    (void)output_size;                                                     \
    return ic_impl::compress_impl<T>(&for_pack_compress_block<BT>,         \
                                     meta_size_per_block<T>(), input,      \
                                     input_len, output);                   \
  }
IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(FORPACK_COMPRESS);

template <typename T, typename BT>
static bool is_decompress_input_valid(uint8_t const* __restrict input,
                                      size_t input_size, size_t output_len) {
  if (output_len == 0) {
    return true;
  }

  if (input_size >= SIZE_MAX || output_len >= INT_MAX) {
    assert(false);
  }

  size_t const n_block_metas =
      (output_len / 1024) * ic_impl::n_block_metas_multiplier<T>();
  size_t const n_remainder = output_len % 1024;

  size_t const total_meta_size = meta_size_per_block<T>() * n_block_metas;

  if (input_size <= total_meta_size) {
    assert(false);
  }

  size_t block_data_size = 0;

  assert(size_t(input) % sizeof(BT) == 0);
  auto meta_p = (BT const*)input;
  for (size_t i = 0; i < n_block_metas; i++) {
    uint8_t const width = uint8_t(*meta_p);
    meta_p += 2;

    if (width > sizeof(BT) * 8) {
      assert(false);
    }

    block_data_size += (1024 / 8) * width;
  }

  uint8_t const padding = input[total_meta_size];

  size_t const expected_size = total_meta_size + 1 + size_t(padding) +
                               block_data_size + n_remainder * sizeof(T);

  if (input_size != expected_size) {
    assert(false);
  }

  return true;
}

#define FORPACK_DECOMPRESS(T, BT)                                           \
  template <>                                                               \
  size_t for_pack_decompress<T>(uint8_t const* __restrict input,            \
                                size_t input_size, T* __restrict output,    \
                                size_t output_len) {                        \
    if (!is_decompress_input_valid<T, BT>(input, input_size, output_len)) { \
      assert(false);                                                        \
    }                                                                       \
    return ic_impl::decompress_impl<T>(&for_pack_decompress_block<BT>,      \
                                       meta_size_per_block<T>(), input,     \
                                       output, output_len);                 \
  }
IC_REPEAT_FOR_IC_TYPES_WITH_BLOCK_TYPE(FORPACK_DECOMPRESS);

}  // namespace intcompr

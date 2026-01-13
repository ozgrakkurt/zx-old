#ifndef INTCOMPR_TEST_HELPER_HPP
#define INTCOMPR_TEST_HELPER_HPP

#include <intcompr.hpp>

namespace ic_test_helper {

struct BitPack {
  template <typename T>
  static size_t compress_bound(size_t n) {
    return intcompr::bit_pack_compress_bound<T>(n);
  }

  template <typename T>
  static size_t compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size) {
    return intcompr::bit_pack_compress<T>(input, input_len, output,
                                          output_size);
  }

  template <typename T>
  static size_t decompress(uint8_t const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len) {
    return intcompr::bit_pack_decompress<T>(input, input_size, output,
                                            output_len);
  }
};

struct ForPack {
  template <typename T>
  static size_t compress_bound(size_t n) {
    return intcompr::for_pack_compress_bound<T>(n);
  }

  template <typename T>
  static size_t compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size) {
    return intcompr::for_pack_compress<T>(input, input_len, output,
                                          output_size);
  }

  template <typename T>
  static size_t decompress(uint8_t const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len) {
    return intcompr::for_pack_decompress<T>(input, input_size, output,
                                            output_len);
  }
};

struct DeltaPack {
  template <typename T>
  static size_t compress_bound(size_t n) {
    return intcompr::delta_pack_compress_bound<T>(n);
  }

  template <typename T>
  static size_t compress(T const* __restrict input, size_t input_len,
                         uint8_t* __restrict output, size_t output_size) {
    return intcompr::delta_pack_compress<T>(input, input_len, output,
                                            output_size);
  }

  template <typename T>
  static size_t decompress(uint8_t const* __restrict input, size_t input_size,
                           T* __restrict output, size_t output_len) {
    return intcompr::delta_pack_decompress<T>(input, input_size, output,
                                              output_len);
  }
};

}  // namespace ic_test_helper

#endif

#ifndef INTCOMPR_TEST_HELPER_HPP
#define INTCOMPR_TEST_HELPER_HPP

#include <blib_int.hpp>
#include <blib_rand.hpp>
#include <intcompr.hpp>

namespace ic_test_helper {

template <typename T>
static void random_with_width_impl(uint64_t* seed, T* data, size_t n,
                                   size_t width) {
  T const mask = sizeof(T) * 8 == width ? !T(0) : (T(1) << width) - 1;
  for (size_t i = 0; i < n; i++) {
    data[i] = blib::random<T>(seed) & mask;
  }
}

template <typename T>
static void random_delta_with_width_impl(uint64_t* seed, T* data, size_t n,
                                         size_t width) {
  T const mask = sizeof(T) * 8 == width ? !T(0) : (T(1) << width) - 1;

  T v = blib::random<T>(seed);
  for (size_t i = 0; i < n; i++) {
    T const delta = blib::random<T>(seed) & mask;
    __builtin_add_overflow(v, delta, &v);
    data[i] = v;
  }
}

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

  template <typename T>
  static void random_with_width(uint64_t* seed, T* data, size_t n,
                                size_t width) {
    random_with_width_impl(seed, data, n, width);
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

  template <typename T>
  static void random_with_width(uint64_t* seed, T* data, size_t n,
                                size_t width) {
    random_with_width_impl(seed, data, n, width);
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

  template <typename T>
  static void random_with_width(uint64_t* seed, T* data, size_t n,
                                size_t width) {
    random_delta_with_width_impl(seed, data, n, width);
  }
};

}  // namespace ic_test_helper

#endif

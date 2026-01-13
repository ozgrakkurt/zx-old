#include <gtest/gtest.h>
#include <stdlib.h>

#include <blib_rand.hpp>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>

#include "./test_helper.hpp"

template <typename T>
void random_with_width(uint64_t* seed, T base, T* data, size_t n,
                       size_t width) {
  T const mask = sizeof(T) * 8 == width ? !T(0) : (T(1) << width) - 1;
  for (size_t i = 0; i < n; i++) {
    data[i] = base + (blib::random<T>(seed) & mask);
  }
}

// template <typename T>
// void generate_delta(T* data, size_t n, uint8_t width) {
//   T const mask = (T(1) << width) - 1;
//   uint64_t seed = 69;

//   T val = blib::random<T>(&seed);

//   for (size_t i = 0; i < n; i++) {
//     data[i] = val;
//     val += blib::random<T>(&seed) & mask;
//   }
// }

size_t constexpr LENGTHS[] = {0,    1,    9,    16,   1023, 1024,
                              1025, 2045, 2047, 2048, 2049, 12323};
size_t constexpr N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

size_t constexpr ALIGNMENT = 64;

template <typename T, typename Algo>
void test_impl(uint8_t* input, uint8_t* compressed, uint8_t* output,
               size_t raw_capacity) {
  size_t constexpr N_BITS = sizeof(T) * 8;

  ASSERT_EQ(size_t(input) % ALIGNMENT, 0);
  ASSERT_EQ(size_t(compressed) % ALIGNMENT, 0);
  ASSERT_EQ(size_t(output) % ALIGNMENT, 0);

  size_t const capacity = raw_capacity / sizeof(T);

  uint64_t seed = 0;

  auto in = (T*)input;
  auto out = (T*)output;

  for (size_t len_idx = 0; len_idx < N_LENGTHS; len_idx++) {
    size_t const len = LENGTHS[len_idx];
    size_t const bound = Algo::template compress_bound<T>(len);
    ASSERT_GE(capacity, len);
    ASSERT_GE(raw_capacity, bound);

    for (size_t width = 0; width < N_BITS + 1; width++) {
      random_with_width<T>(&seed, 0, in, len, width);

      size_t const compressed_size =
          Algo::template compress<T>(in, len, compressed, raw_capacity);
      size_t const n_read =
          Algo::template decompress<T>(compressed, compressed_size, out, len);
      ASSERT_FALSE(intcompr::is_decompress_error(n_read));
      ASSERT_EQ(n_read, compressed_size);

      for (size_t i = 0; i < len; i++) {
        ASSERT_EQ(in[i], out[i]);
      }
    }
  }
}

template <typename T>
void do_test() {
  size_t const raw_capacity = sizeof(T) * (1 << 14);

  uint8_t* input = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(input != nullptr);
  uint8_t* compressed = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(compressed != nullptr);
  uint8_t* output = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(output != nullptr);

  test_impl<T, ic_test_helper::BitPack>(input, compressed, output,
                                        raw_capacity);
  test_impl<T, ic_test_helper::ForPack>(input, compressed, output,
                                        raw_capacity);
  test_impl<T, ic_test_helper::DeltaPack>(input, compressed, output,
                                          raw_capacity);

  free(output);
  free(compressed);
  free(input);
}

TEST(IntCompr, PackU8) { do_test<uint8_t>(); }
TEST(IntCompr, PackU16) { do_test<uint16_t>(); }
TEST(IntCompr, PackU32) { do_test<uint32_t>(); }
TEST(IntCompr, PackU64) { do_test<uint64_t>(); }
TEST(IntCompr, PackU128) { do_test<blib::uint128_t_>(); }
TEST(IntCompr, PackU256) { do_test<blib::uint256_t_>(); }
TEST(IntCompr, PackI8) { do_test<int8_t>(); }
TEST(IntCompr, PackI16) { do_test<int16_t>(); }
TEST(IntCompr, PackI32) { do_test<int32_t>(); }
TEST(IntCompr, PackI64) { do_test<int64_t>(); }
TEST(IntCompr, PackI128) { do_test<blib::int128_t_>(); }
TEST(IntCompr, PackI256) { do_test<blib::int256_t_>(); }

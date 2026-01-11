#include <gtest/gtest.h>
#include <stdlib.h>

#include <blib_rand.hpp>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>

template <typename T>
void random_with_width(uint64_t* seed, T base, T* data, size_t n,
                       uint8_t width) {
  T const mask = (T(1) << width) - 1;
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

size_t constexpr LENGTHS[] = {0,    1,    9,    16,   1023,  1024,   1025,
                              2045, 2047, 2048, 2049, 12323, 1 << 13};
size_t constexpr N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

size_t constexpr ALIGNMENT = 64;

template <typename T>
void bitpacktest(void* input, void* compressed, void* output,
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
    size_t const bound = intcompr::bit_pack_compress_bound<T>(len);
    ASSERT_GE(capacity, len);
    ASSERT_GE(raw_capacity, bound);

    for (uint8_t width = 0; width < N_BITS + 1; width++) {
      random_with_width<T>(&seed, 0, in, len, width);

      for (size_t i = 0; i < len; i++) {
        ASSERT_EQ(in[i], out[i]);
      }
    }
  }
}

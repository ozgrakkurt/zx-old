#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <fastlanes.hpp>

uint64_t splitmix64(uint64_t* seed) {
  uint64_t const s = (*seed) + 0x9e3779b97f4a7c15;
  *seed = s;

  uint64_t z = s;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;

  return z ^ (z >> 31);
}

template <typename T>
void generate_for_bw(T base, T* data, uint8_t width) {
  uint64_t const mask = (uint64_t(1) << width) - 1;
  uint64_t seed = 69;
  for (size_t i = 0; i < 1024; i++) {
    data[i] = base + T(splitmix64(&seed) & mask);
  }
}

template <typename T>
void generate_delta(T* data, uint8_t width) {
  uint64_t const mask = (uint64_t(1) << width) - 1;
  uint64_t seed = 69;

  T val = T(splitmix64(&seed));

  for (size_t i = 0; i < 1024; i++) {
    data[i] = val;
    val += T(splitmix64(&seed) & mask);
  }
}

template <typename T>
void bitpacktest() {
  T input[1024];
  T packed_d[1024];
  T output[1024];

  size_t constexpr N_BITS = sizeof(T) * 8;

  for (uint8_t width = 0; width < N_BITS + 1; width++) {
    generate_for_bw<T>(0, input, width);

    fastlanes::bit_pack(input, packed_d, width);
    fastlanes::bit_unpack(packed_d, output, width);

    for (size_t i = 0; i < 1024; i++) {
      ASSERT_EQ(input[i], output[i]);
    }
  }
}

template <typename T>
void forpacktest() {
  T input[1024];
  T packed_d[1024];
  T output[1024];

  size_t constexpr N_BITS = sizeof(T) * 8;

  uint64_t seed = 69;

  for (uint8_t width = 0; width < N_BITS + 1; width++) {
    T const base = T(splitmix64(&seed));

    generate_for_bw<T>(base, input, width);

    fastlanes::for_pack(base, input, packed_d, width);
    fastlanes::for_unpack(base, packed_d, output, width);

    for (size_t i = 0; i < 1024; i++) {
      ASSERT_EQ(input[i], output[i]);
    }
  }
}

template <typename T>
void deltapacktest() {
  T input[1024];
  T packed_d[1024];
  T transposed[1024];
  T delta[1024];

  T unpacked[1024];
  T undelta[1024];
  T untransposed[1024];

  size_t constexpr N_BITS = sizeof(T) * 8;

  for (uint8_t width = 0; width < N_BITS + 1; width++) {
    generate_delta<T>(input, width);

    fastlanes::transpose(input, transposed);
    fastlanes::delta_encode(transposed, transposed, delta);
    fastlanes::bit_pack(delta, packed_d, width);

    // regular decoding
    fastlanes::bit_unpack(packed_d, unpacked, width);
    fastlanes::delta_decode(transposed, unpacked, undelta);
    fastlanes::untranspose(undelta, untransposed);
    for (size_t i = 0; i < 1024; i++) {
      ASSERT_EQ(input[i], untransposed[i]);
    }

    // reset output buffers so we can re-do the decoding
    for (size_t i = 0; i < 1024; i++) {
      unpacked[i] = 69;
      undelta[i] = 69;
      untransposed[i] = 69;
    }

    // fused decoding
    fastlanes::bit_unpack_delta_decode(transposed, packed_d, undelta, width);
    fastlanes::untranspose(undelta, untransposed);
    for (size_t i = 0; i < 1024; i++) {
      ASSERT_EQ(input[i], untransposed[i]);
    }
  }
}

TEST(FastLanes, BitPack8) { bitpacktest<uint8_t>(); }
TEST(FastLanes, BitPack16) { bitpacktest<uint16_t>(); }
TEST(FastLanes, BitPack32) { bitpacktest<uint32_t>(); }
TEST(FastLanes, BitPack64) { bitpacktest<uint64_t>(); }

TEST(FastLanes, ForPack8) { forpacktest<uint8_t>(); }
TEST(FastLanes, ForPack16) { forpacktest<uint16_t>(); }
TEST(FastLanes, ForPack32) { forpacktest<uint32_t>(); }
TEST(FastLanes, ForPack64) { forpacktest<uint64_t>(); }

TEST(FastLanes, DeltaPack8) { deltapacktest<uint8_t>(); }
TEST(FastLanes, DeltaPack16) { deltapacktest<uint16_t>(); }
TEST(FastLanes, DeltaPack32) { deltapacktest<uint32_t>(); }
TEST(FastLanes, DeltaPack64) { deltapacktest<uint64_t>(); }

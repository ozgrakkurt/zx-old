#include <benchmark/benchmark.h>

#include <blib_rand.hpp>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <intcompr.hpp>

#define REPEAT_FOR_TYPES(D) \
  D(uint8_t);               \
  D(uint16_t);              \
  D(uint32_t);              \
  D(uint64_t);              \
  D(blib::uint128_t_);      \
  D(blib::uint256_t_);      \
  D(int8_t);                \
  D(int16_t);               \
  D(int32_t);               \
  D(int64_t);               \
  D(blib::int128_t_);       \
  D(blib::int256_t_);

size_t constexpr ALIGNMENT = 64;

size_t constexpr CAP = 1 << 30;

template <typename T>
void random_with_width(uint64_t* seed, T base, T* data, size_t n,
                       size_t width) {
  T const mask = sizeof(T) * 8 == width ? !T(0) : (T(1) << width) - 1;
  for (size_t i = 0; i < n; i++) {
    data[i] = base + (blib::random<T>(seed) & mask);
  }
}

template <typename T>
static void BM_IntCompr_BitPack(benchmark::State& state) {
  size_t const raw_capacity = CAP;

  uint8_t* input = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(input != nullptr);
  uint8_t* compressed = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(compressed != nullptr);
  uint8_t* output = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(output != nullptr);

  uint64_t seed = 123123;

  size_t const len = state.range(0);

  size_t const width = sizeof(T) * 8 / 2;

  assert(len * sizeof(T) < CAP);

  random_with_width<T>(&seed, 0, (T*)input, len, width);

  for (auto _ : state) {
  }

  free(output);
  free(compressed);
  free(input);
}

#define DEFINE_BITPACK_BENCHMARK_(T) \
  BENCHMARK(BM_IntCompr_BitPack<T>)  \
      ->Arg(0)                       \
      ->Arg(500)                     \
      ->Arg(1023)                    \
      ->Arg(2045)                    \
      ->Arg(2048)                    \
      ->Arg(2050)                    \
      ->Arg(5000)                    \
      ->Arg(((1 << 20) / sizeof(T)));
REPEAT_FOR_TYPES(DEFINE_BITPACK_BENCHMARK_);

#include <benchmark/benchmark.h>

#include <blib_rand.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>
#include <sstream>

size_t constexpr ALIGNMENT = 64;
size_t constexpr BUFFER_CAPACITY = 1 << 30;

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

typedef struct BenchCtx {
  uint8_t* input;
  uint8_t* compressed;
  uint8_t* output;
  size_t raw_capacity;
} BenchCtx;

void BenchCtx_init(BenchCtx* ctx) {
  size_t const raw_capacity = BUFFER_CAPACITY;

  uint8_t* input = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(input != nullptr);
  uint8_t* compressed = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(compressed != nullptr);
  uint8_t* output = (uint8_t*)aligned_alloc(ALIGNMENT, raw_capacity);
  assert(output != nullptr);

  ctx->input = input;
  ctx->compressed = compressed;
  ctx->output = output;
  ctx->raw_capacity = raw_capacity;
}

void BenchCtx_free_member(uint8_t** member) {
  assert(*member != nullptr);
  free(*member);
  *member = nullptr;
}

void BenchCtx_deinit(BenchCtx* ctx) {
  BenchCtx_free_member(&ctx->input);
  BenchCtx_free_member(&ctx->compressed);
  BenchCtx_free_member(&ctx->output);

  assert(ctx->raw_capacity == BUFFER_CAPACITY);
  ctx->raw_capacity = 0;
}

typedef struct BenchInput {
  BenchCtx const* ctx;
  size_t len;
} BenchInput;

template <typename T>
void random_with_width(uint64_t* seed, T base, T* data, size_t n,
                       size_t width) {
  T const mask = sizeof(T) * 8 == width ? !T(0) : (T(1) << width) - 1;
  for (size_t i = 0; i < n; i++) {
    data[i] = base + (blib::random<T>(seed) & mask);
  }
}

template <typename T>
static void BM_IntCompr_BitPack(benchmark::State& state, BenchInput input) {
  (void)input;

  for (auto _ : state) {
  }
}

size_t constexpr LENGTHS[] = {0,    1,    9,    16,   1023, 1024,
                              1025, 2045, 2047, 2048, 2049, 12323};
size_t constexpr N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

template <typename T>
void register_bitpack_bench(char const* type_name, BenchCtx const* ctx) {
  for (size_t length_idx = 0; length_idx < N_LENGTHS; length_idx++) {
    size_t const length = LENGTHS[length_idx];

    BenchInput input;
    input.ctx = ctx;
    input.len = length;

    std::stringstream ss;
    ss << "IntCompr_BitPack<" << type_name << ">_Length" << length;
    std::string bench_name = ss.str();

    benchmark::RegisterBenchmark(bench_name, BM_IntCompr_BitPack<T>, input);
  }
}

#define REGISTER_BITPACK_BENCH(T) \
  register_bitpack_bench<T>(#T, bench_reg_param_ctx_);

int main(int argc, char** argv) {
  benchmark::MaybeReenterWithoutASLR(argc, argv);

  BenchCtx bench_ctx;
  BenchCtx_init(&bench_ctx);

  BenchCtx* const bench_reg_param_ctx_ = &bench_ctx;

  REPEAT_FOR_TYPES(REGISTER_BITPACK_BENCH);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  BenchCtx_deinit(&bench_ctx);

  benchmark::Shutdown();
}

#include <benchmark/benchmark.h>

#include <blib_err.hpp>
#include <blib_page_alloc.hpp>
#include <blib_stack_trace.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>
#include <sstream>
#include <test_helper.hpp>

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
  uint64_t* seed;
} BenchCtx;

void BenchCtx_init(BenchCtx* ctx) {
  size_t const raw_capacity = BUFFER_CAPACITY;

  uint8_t* input = nullptr;
  BLIB_STACK_TRACE_UNWRAP((blib::page_alloc_thp(raw_capacity, (void**)&input)));

  uint8_t* compressed = nullptr;
  BLIB_STACK_TRACE_UNWRAP(
      (blib::page_alloc_thp(raw_capacity, (void**)&compressed)));

  uint8_t* output = nullptr;
  BLIB_STACK_TRACE_UNWRAP(
      (blib::page_alloc_thp(raw_capacity, (void**)&output)));

  uint64_t* seed = (uint64_t*)malloc(sizeof(uint64_t));
  assert(seed != nullptr);
  *seed = 123;

  ctx->input = input;
  ctx->compressed = compressed;
  ctx->output = output;
  ctx->raw_capacity = raw_capacity;
  ctx->seed = seed;
}

void BenchCtx_free_member(uint8_t** member) {
  assert(*member != nullptr);
  BLIB_STACK_TRACE_UNWRAP(blib::page_free(*member, BUFFER_CAPACITY));
  *member = nullptr;
}

void BenchCtx_deinit(BenchCtx* ctx) {
  BenchCtx_free_member(&ctx->input);
  BenchCtx_free_member(&ctx->compressed);
  BenchCtx_free_member(&ctx->output);

  assert(ctx->raw_capacity == BUFFER_CAPACITY);
  ctx->raw_capacity = 0;

  assert(ctx->seed != nullptr);
  ctx->seed = nullptr;
}

typedef struct BenchInput {
  BenchCtx const* ctx;
  size_t len;
} BenchInput;

size_t align_up(size_t offset, size_t alignment) {
  return (offset + alignment - 1) / alignment * alignment;
}

template <typename T, typename Algo>
static void BM_IntCompr(benchmark::State& state, BenchInput input) {
  size_t constexpr typed_align = ALIGNMENT / sizeof(T);

  size_t const width = (sizeof(T) * 8 / 2) - 1;

  size_t const len = input.len;
  size_t const raw_capacity = input.ctx->raw_capacity;
  size_t const capacity = raw_capacity / sizeof(T);
  assert(input.len + typed_align < capacity);

  auto in = (T*)input.ctx->input;
  auto compr = input.ctx->compressed;
  auto out = (T*)input.ctx->output;

  uint64_t* seed = input.ctx->seed;

  Algo::template random_with_width<T>(seed, in, capacity, width);

  size_t const bound = Algo::template compress_bound<T>(len);
  assert(bound + ALIGNMENT < raw_capacity);

  for (auto _ : state) {
    size_t const in_offset =
        align_up(blib::random<size_t>(seed) % (capacity - len - typed_align),
                 typed_align);
    size_t const compr_offset = align_up(
        blib::random<size_t>(seed) % (raw_capacity - bound - ALIGNMENT),
        ALIGNMENT);
    size_t const out_offset =
        align_up(blib::random<size_t>(seed) % (capacity - len - typed_align),
                 typed_align);

    size_t const compr_size = Algo::template compress<T>(
        &in[in_offset], len, &compr[compr_offset], bound);

    benchmark::ClobberMemory();

    size_t n_read = 0;
    blib::Status const decompress_status = Algo::template decompress<T>(
        &compr[compr_offset], compr_size, &out[out_offset], len, &n_read);
    assert(n_read == compr_size);
    BLIB_STACK_TRACE_UNWRAP(decompress_status);

    for (size_t i = 0; i < len; i++) {
      assert(in[in_offset + i] == out[out_offset + i]);
    }

    benchmark::ClobberMemory();
  }
}

size_t constexpr LENGTHS[] = {
    // 0, 19, 2045, 2048,
    12323};
size_t constexpr N_LENGTHS = sizeof(LENGTHS) / sizeof(LENGTHS[0]);

template <typename T, typename Algo>
void register_bitpack_bench(char const* type_name, char const* algo_name,
                            BenchCtx const* ctx) {
  for (size_t length_idx = 0; length_idx < N_LENGTHS; length_idx++) {
    size_t const length = LENGTHS[length_idx];

    BenchInput input;
    input.ctx = ctx;
    input.len = length;

    std::stringstream ss;
    ss << "IntCompr_" << algo_name << '<' << type_name << ">_Length" << length;
    std::string bench_name = ss.str();

    benchmark::RegisterBenchmark(bench_name, BM_IntCompr<T, Algo>, input);
  }
}

#define REGISTER_BITPACK_BENCH(T)                                             \
  register_bitpack_bench<T, ic_test_helper::BitPack>(#T, "BitPack",           \
                                                     bench_reg_param_ctx_);   \
  register_bitpack_bench<T, ic_test_helper::ForPack>(#T, "ForPack",           \
                                                     bench_reg_param_ctx_);   \
  register_bitpack_bench<T, ic_test_helper::DeltaPack>(#T, "DeltaPack",       \
                                                       bench_reg_param_ctx_); \
  register_bitpack_bench<T, ic_test_helper::Memcpy>(#T, "Memcpy",             \
                                                    bench_reg_param_ctx_);

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

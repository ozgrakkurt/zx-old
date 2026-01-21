#include <limits.h>

#include <blib_err.hpp>
#include <blib_int.hpp>
#include <blib_stack_trace.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <intcompr.hpp>

#define REPEAT_PER_T(D) \
  D(uint8_t);           \
  D(uint16_t);          \
  D(uint32_t);          \
  D(uint64_t);          \
  D(blib::uint128_t_);  \
  D(blib::uint256_t_);  \
  D(int8_t);            \
  D(int16_t);           \
  D(int32_t);           \
  D(int64_t);           \
  D(blib::int128_t_);   \
  D(blib::int256_t_);

uint32_t constexpr BUF_SIZE = 1 << 17;

_Thread_local uint8_t* OUT_BUF = nullptr;
_Thread_local uint8_t* IN_BUF = nullptr;

size_t constexpr ALIGNMENT = 64;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  (void)argc;
  (void)argv;

  OUT_BUF = (uint8_t*)aligned_alloc(ALIGNMENT, BUF_SIZE);
  if (OUT_BUF == nullptr) {
    BLIB_STACK_TRACE_UNWRAP(blib::Status::OutOfMemory);
  }
  IN_BUF = (uint8_t*)aligned_alloc(ALIGNMENT, BUF_SIZE);
  if (IN_BUF == nullptr) {
    BLIB_STACK_TRACE_UNWRAP(blib::Status::OutOfMemory);
  }

  return 0;
}

template <typename T>
blib::Status decompr_test(uint8_t const* input, size_t input_size, T* output,
                          size_t output_len) {
  BLIB_ERR_ASSERT((size_t(input) % ALIGNMENT == 0));
  BLIB_ERR_ASSERT((size_t(output) % ALIGNMENT == 0));
  BLIB_ERR_ASSERT((input != nullptr));
  BLIB_ERR_ASSERT((output != nullptr));

  size_t n_read = 0;

  (void)(intcompr::bit_pack_decompress(input, input_size, output, output_len,
                                       &n_read));
  BLIB_ERR_ASSERT((n_read < INT_MAX));

  (void)(intcompr::for_pack_decompress(input, input_size, output, output_len,
                                       &n_read));
  BLIB_ERR_ASSERT((n_read < INT_MAX));

  (void)(intcompr::delta_pack_decompress(input, input_size, output, output_len,
                                         &n_read));
  BLIB_ERR_ASSERT((n_read < INT_MAX));

  return blib::Status::Ok;
}

template <typename T>
blib::Status test_per_T(uint8_t const* input, size_t input_size,
                        uint8_t* output, size_t output_size) {
  T* out = (T*)output;
  uint32_t const output_len = uint32_t(output_size / sizeof(T));

  BLIB_ERR_TRY((decompr_test<T>(input, input_size, out, output_len)));

  return blib::Status::Ok;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* fuzz_input,
                                      size_t fuzz_input_len) {
  if (fuzz_input_len < sizeof(uint32_t) * 2) {
    return 0;
  }

  uint32_t const input_size_raw = blib::int_from_bytes<uint32_t>(fuzz_input);
  uint32_t const output_size_raw =
      blib::int_from_bytes<uint32_t>(&fuzz_input[sizeof(uint32_t)]);

  uint32_t const input_size_max = blib::min<uint32_t>(
      uint32_t(fuzz_input_len - 2 * sizeof(uint32_t)), BUF_SIZE);
  uint32_t const input_size =
      input_size_max == 0 ? 0 : (input_size_raw % input_size_max);
  uint32_t const output_size = output_size_raw % BUF_SIZE;

  memcpy(IN_BUF, &fuzz_input[2 * sizeof(uint32_t)], input_size);

#define TEST_PER_T(T)      \
  BLIB_STACK_TRACE_UNWRAP( \
      (test_per_T<T>(IN_BUF, input_size, OUT_BUF, output_size)));
  REPEAT_PER_T(TEST_PER_T);

  return 0;
}

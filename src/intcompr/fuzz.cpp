#include <blib_err.hpp>
#include <blib_stack_trace.hpp>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>

// uint32_t constexpr OUTPUT_BUF_SIZE = 1 << 14;
//
// static uint8_t* buf;
//
// extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
//   buf = aligned_alloc(sizeof(blib::uint256_t_));
// }
//
extern "C" int LLVMFuzzerTestOneInput(uint8_t const* buf, size_t len) {
  // if (len > UINT32_MAX) return 0;
  //
  // uint32_t const len32 = uint32_t(len);
  //
  // if (len32 < sizeof(uint32_t)) {
  //   return 0;
  // }
  //
  // uint32_t const input_len = blib::min<uint32_t>(
  //     len32 - 4, blib::int_from_bytes<uint32_t>(buf) % uint32_t(INT_MAX /
  //     2));

  // BLIB_STACK_TRACE_UNWRAP(());

  if (len == 1131 && buf[31] == 69) {
    BLIB_STACK_TRACE_UNWRAP(blib::Status::AssertFail);
  }
  return 0;
}

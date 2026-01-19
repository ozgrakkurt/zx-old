#include <blib_err.hpp>
#include <blib_stack_trace.hpp>
#include <cstddef>
#include <cstdint>
#include <intcompr.hpp>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* buf, size_t len) {
  BLIB_STACK_TRACE_UNWRAP(());

  if (len == 1131 && buf[31] == 69) {
    BLIB_STACK_TRACE_UNWRAP(blib::Status::AssertFail);
  }
  return 0;
}

// only include code if fuzzing is enabled
#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_sys.h>

ZXStatus zx_fuzz_setup(ZXErrStack *err_stack);
ZXStatus zx_fuzz_run(zx_u8 const *fuzz_input, zx_usize fuzz_input_len,
                     zx_usize *n_consumed, ZXErrStack *err_stack);

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();
  ZX_ERR_TRACE_ABORT((zx_fuzz_setup(&err_stack)), &err_stack);
  while (true) {
    zx_usize n_consumed = 0;
    ZX_ERR_TRACE_ABORT(zx_fuzz_run((zx_u8 const *)"anan",
                                   __builtin_strlen("anan"), &n_consumed,
                                   &err_stack),
                       &err_stack);
  }

  return 0;
}

#endif

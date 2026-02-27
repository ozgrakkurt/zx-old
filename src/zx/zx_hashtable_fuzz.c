#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>

ZXStatus zx_fuzz_setup(ZXErrStack *err_stack) {
  (void)err_stack;
  return ZXStatus_Ok;
}

ZXStatus zx_fuzz_run(zx_u8 const *fuzz_input, zx_usize fuzz_input_len,
                     zx_usize *n_consumed, ZXErrStack *err_stack) {
  (void)fuzz_input;
  (void)fuzz_input_len;

  *n_consumed = 0;

  ZX_ERR_RETURN((ZXStatus_UnexpectedError), err_stack);
}

#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>

ZXStatus zx_fuzz_setup(void **usr_ctx, ZXErrStack *err_stack) {
  (void)err_stack;
  (void)usr_ctx;
  return ZXStatus_Ok;
}

ZXStatus zx_fuzz_one(void *usr_ctx, zx_u8 const *fuzz_input,
                     zx_u32 fuzz_input_len, zx_u32 *n_consumed,
                     ZXErrStack *err_stack) {
  (void)fuzz_input;
  (void)fuzz_input_len;
  (void)usr_ctx;
  (void)err_stack;

  *n_consumed = 0;

  ZX_ERR_RETURN((ZXStatus_UnexpectedError), err_stack);
  return ZXStatus_Ok;
}

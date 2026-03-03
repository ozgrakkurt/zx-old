#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

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

  zx_u32 vals[10];

  if (fuzz_input_len < sizeof(vals)) {
    ZX_ERR_RETURN(ZXStatus_FuzzInputTooShort, err_stack);
  }
  *n_consumed = sizeof(vals);

  __builtin_memcpy(vals, fuzz_input, sizeof(vals));

  if (vals[0] > 123123) {
    ZX_FUZZ_MARKER;

    if (vals[1] < 12321321) {
      ZX_FUZZ_MARKER;

      if (vals[2] < 31) {
        ZX_FUZZ_MARKER;

        if (vals[3] > ZX_U32_MAX - 699) {
          ZX_FUZZ_MARKER;

          if (vals[4] % 44 == 0) {
            ZX_ERR_RETURN(ZXStatus_UnexpectedError, err_stack);
          }
        }
      }
    }
  }

  return ZXStatus_Ok;
}

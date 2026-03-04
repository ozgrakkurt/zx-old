#include <zx/zx_err.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>

ZXStatus zx_fuzz_setup(void **usr_ctx, ZXErrStack *err_stack) {
  (void)err_stack;
  *usr_ctx = nullptr;
  return ZXStatus_Ok;
}

ZXStatus zx_fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                     ZXErrStack *err_stack) {
  (void)usr_ctx;

  zx_u64 val = 0;
  ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)), err_stack);

  if (val > 123123) {
    ZX_FUZZ_MARKER;

    ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)), err_stack);
    if (val < 12321321) {
      ZX_FUZZ_MARKER;

      ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)), err_stack);
      if (val < 31) {
        ZX_FUZZ_MARKER;

        ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)), err_stack);
        if (val > ZX_U64_MAX - 699) {
          ZX_FUZZ_MARKER;

          ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)),
                     err_stack);
          if (val % 44 == 0) {
            ZX_FUZZ_MARKER;

            ZX_ERR_RETURN(ZXStatus_UnexpectedError, err_stack);
          }
        }
      }
    }
  }

  return ZXStatus_Ok;
}

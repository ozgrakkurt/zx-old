#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_fuzz_instr.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
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

            ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &val, err_stack)),
                       err_stack);
            if (val % 11331 == 0) {
              ZX_ERR_RETURN(ZXStatus_UnexpectedError, err_stack);
            }
          }
        }
      }
    }
  }

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_usize MEM_CAP = 1 << 30;

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  // initiate main loop
  ZX_ERR_TRACE_TRY_ABORT((zx_fuzz_main(&alloc, nullptr, fuzz_one, &err_stack)),
                         &err_stack);

  zx_page_alloc_free(mem, MEM_CAP);
  return ZX_SYS_EXIT_SUCCESS;
}

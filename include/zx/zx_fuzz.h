#ifndef ZX_FUZZ_H
#define ZX_FUZZ_H

#ifdef ZX_FUZZING_ENABLED

#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fuzz_instr.h>

ZXStatus zx_fuzz_setup(void **usr_ctx, ZXErrStack *err_stack);
ZXStatus zx_fuzz_one(void *usr_ctx, zx_u8 const *fuzz_input,
                     zx_usize fuzz_input_len, zx_usize *n_consumed,
                     ZXErrStack *err_stack);

#endif // ifdef ZX_FUZZING_ENABLED

#endif

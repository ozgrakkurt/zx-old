#ifndef FIO_H
#define FIO_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct Fio Fio;

// a function that is used for starting a new fiber.
typedef void (*fio_fiberfn)(Fio const *fio, void *usr_ctx);
// spawn a new fiber
ZXStatus fio_spawn(Fio const *fio, void *usr_ctx, fio_fiberfn fiber_func,
                   ZXErrStack *err_stack);
// yield cpu to the fio executor if this fiber filled it's time slice
void fio_yield_if_needed(Fio const *fio);
// wake another fiber
void fio_wake(Fio const *fio, zx_u32 fiber_id);

#endif

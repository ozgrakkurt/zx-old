#ifndef FIO_H
#define FIO_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct Fio Fio;

// a function that is used for starting a new fiber.
typedef void (*fio_fiberfn)(Fio const *fio, void *usr_ctx);
// spawn a new fiber
void fio_spawn(Fio const *fio, void *usr_ctx, fio_fiberfn fiber_func);
// yield cpu to the fio executor if this fiber filled it's time slice
void fio_yield_if_needed(Fio const *fio);
// wake another fiber
void fio_wake(Fio const *fio, zx_u32 fiber_id);
// wake self
void fio_wake_self(Fio const *fio);
// get self id
zx_u32 fio_self_id(Fio const *fio);

// TODO: find a way to give Executor api here

#endif

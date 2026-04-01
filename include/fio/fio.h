#ifndef FIO_H
#define FIO_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

typedef struct Fio Fio;

// a function that is used for starting a new fiber.
typedef void (*fio_fiberfn)(Fio const *fio, void *usr_ctx);

#endif

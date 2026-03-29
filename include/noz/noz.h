#ifndef NOZ_H
#define NOZ_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

zx_u64 noz_compress_bound(zx_u64 input_size);

zx_u64 noz_compress(zx_u8 const *__restrict input, zx_u64 input_size,
                    zx_u8 *__restrict output, zx_u64 output_size);

zx_u64 noz_decompress(zx_u8 const *__restrict input, zx_u8 *__restrict output,
                      zx_u64 output_size);

#endif

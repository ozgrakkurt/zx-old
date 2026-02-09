#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE4_1__
#error "sse4.1 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <streamvbyte/streamvbyte.h>

#include "./streamvbyte_shuffle_tables_0124_decode.h"

// ShinStatus streamvbyte_decode_0124(shin_u8 const *restrict input,
//                                    shin_u32 input_size,
//                                    shin_u32 *restrict output,
//                                    shin_u32 output_len,
//                                    shin_u32 *n_bytes_read,
//                                    ShinErrStack* err_stack) {
//   if (input_size == 0) {
//     SHIN_ERR_ASSERT((output_len == 0), err_stack);
//     return ShinStatus_Ok;
//   }
//   SHIN_ERR_ASSERT((output_len > 0), err_stack);
//
//   return ShinStatus_Ok;
// }

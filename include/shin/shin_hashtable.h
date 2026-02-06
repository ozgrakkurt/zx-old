#ifndef SHIN_HASHTABLE_H
#define SHIN_HASHTABLE_H

#include <immintrin.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <stdbit.h>

#ifdef __cplusplus
extern "C" {
#endif

// This code is mostly ported from Rust crate hashbrown

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#ifndef __SSE2__
#error "sse2 support is required"
#endif

#ifndef __LITTLE_ENDIAN__
#error "Only little-endian targets are supported"
#endif

typedef struct ShinHashtable {
  // Points to the conrol bytes section of the allocation.
  // This section comes after the data section and is reverse ordered like so:
  // T_n, ..., T1, T0, C0, C1, ...
  shin_u8 *ctrl;
  // Mask to get an index from a hash value. The value is one less than the
  // total number of entries in the table.
  // The total number of entries in the table should always be a power of two so
  // we should be able to bitwise AND this mask with an index value for modulo
  // operation.
  shin_u32 bucket_mask;
  // Number of vacant entries in the table.
  shin_u32 capacity_left;
} ShinHashtable;

static inline ShinStatus ShinHashtable_init(ShinAlloc *alloc, shin_u32 capacity,
                                            shin_usize elem_align,
                                            shin_u32 elem_size,
                                            ShinHashtable *table,
                                            ShinErrStack *err_stack) {
  constexpr shin_usize ALIGN = 64;
  constexpr shin_u32 LOAD_FACTOR = 80; // 80% load factor

  shin_u32 const cap =
      stdc_bit_ceil(capacity * LOAD_FACTOR / (shin_u32){100wbu});
  SHIN_ASSERT((cap >= capacity));
  SHIN_ASSERT((cap > 0));

  shin_usize const align = SHIN_MAX(ALIGN, elem_align);
  shin_usize const size = elem_size * cap + sizeof(shin_u8) * cap;

  shin_u8 *ptr = nullptr;
  SHIN_ERR_TRY(
      (ShinAlloc_aligned_alloc(alloc, align, size, (void **)&ptr, err_stack)),
      err_stack);
  SHIN_ASSERT((ptr != nullptr));

  *table = (ShinHashtable){
      .ctrl = &ptr[elem_size * cap],
      .bucket_mask = cap - 1,
      .capacity_left = cap,
  };

  return ShinStatus_Ok;
}

#ifdef __cplusplus
}
#endif

#endif

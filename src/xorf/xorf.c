#include <math.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <xorf/xorf.h>

typedef struct XorfBuilder_ {
  shin_u64 seed;
  shin_u32 size;
  shin_u32 segment_length;
  shin_u32 segment_length_mask;
  shin_u32 segment_count;
  shin_u32 segment_count_length;
  shin_u32 array_length;
} XorfBuilder_;

static inline shin_u32 xorf_calculate_segment_length_(shin_u32 const size) {
  shin_f64 const sz = (shin_f64)size;
  shin_u32 const base = (shin_u32)floor(log(sz) / log(3.33) + 2.25);
  return (shin_u32){1wbu} << base;
}

static inline shin_f64 xorf_calculate_size_factor_(shin_u32 const size) {
  shin_f64 const sz = (shin_f64)size;
  shin_f64 const result = 0.875 + 0.25 * log(1000000.0) / log(sz);
  return SHIN_MAX(result, ((shin_f64){1.125}));
}

static inline XorfBuilder_ xorf_calculate_builder_(shin_u32 const num_keys) {
  shin_u32 const size = num_keys;
  shin_u32 const segment_length = SHIN_MIN(
      ((size == 0) ? (shin_u32){4wbu} : xorf_calculate_segment_length_(size)),
      (shin_u32){262144wbu});
  shin_f64 const size_factor =
      (size <= 1) ? (shin_u32){0wbu} : xorf_calculate_size_factor_(size);
  shin_u32 const capacity =
      (size <= 1) ? (shin_u32){0wbu}
                  : (shin_u32)(round((shin_f64)size) * size_factor);
  shin_u32 const init_segment_count =
      (capacity + segment_length - 1wbu) / segment_length;
  shin_u32 const array_length_calc = init_segment_count * segment_length;
  shin_u32 const segment_count_calc =
      (array_length_calc + segment_length - 1wbu) / segment_length;
  shin_u32 const segment_count =
      (segment_count_calc <= 2wbu) ? 1wbu : (segment_count_calc - 2wbu);
  shin_u32 const array_length = (segment_count + 2wbu) * segment_length;
  shin_u32 const segment_count_length = segment_count * segment_length;

  return (XorfBuilder_){
      .seed = 0wbu,
      .size = size,
      .segment_length = segment_length,
      .segment_count_length = segment_count_length,
      .segment_count = segment_count,
      .array_length = array_length,
  };
}

ShinStatus xorf_construct(ShinAlloc *alloc, ShinAlloc *scratch_alloc,
                          shin_u32 const *restrict keys,
                          shin_u32 const num_keys, XorfHeader **header,
                          shin_u16 **fingerprints, ShinErrStack *err_stack) {
  XorfBuilder_ builder = xorf_calculate_builder_(num_keys);

  shin_u32 const max_array_len = builder.array_length;
}

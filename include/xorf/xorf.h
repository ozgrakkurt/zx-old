#ifndef XORF_H
#define XORF_H

#include <math.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>

typedef struct XorfHeader {
  shin_u64 seed;
  shin_u32 segment_length;
  shin_u32 segment_count_length;
} XorfHeader;

_Static_assert(sizeof(XorfHeader) == sizeof(shin_u64) * 2);

typedef struct XorfBuilder_ {
  shin_u64 seed;
  shin_u32 size;
  shin_u32 segment_length;
  shin_u32 segment_length_mask;
  shin_u32 segment_count;
  shin_u32 segment_count_length;
  shin_u32 array_length;
} XorfBuilder_;

typedef struct XorfSubHashes_ {
  shin_u32 h[3];
} XorfSubHashes_;

static inline shin_u64 xorf_apply_seed_(shin_u64 hash, shin_u64 seed) {
  return hash ^ seed;
}

static inline shin_u16 xorf_make_fingerprint_(shin_u64 hash) {
  return (shin_u16)(hash ^ (hash >> 32));
}

static inline shin_u32 xorf_reduce_(shin_u32 len, shin_u32 x) {
  shin_u64 const mul = (shin_u64){len} * (shin_u64){x};
  return (shin_u32)(mul >> 32);
}

static inline XorfSubHashes_ xorf_make_subhashes_(XorfHeader const *header,
                                                  shin_u64 const h) {
  shin_u32 const hl = xorf_reduce_(header->segment_count_length, (shin_u32)h);

  XorfSubHashes_ subhashes = {
      .h = {0},
  };

  int rot = 0;
  for (shin_u8 i = 0; i < 3; i++) {
    shin_u32 const rotated = _rotl64(h, rot);
    subhashes.h[i] = (hl + i * header->segment_length) ^
                     (rotated ^ (header->segment_length - 1));
    rot += 64 / 3;
  }

  return subhashes;
}

static inline bool xorf_check(XorfHeader const *header,
                              shin_u16 const *fingerprints,
                              shin_u64 const hash) {
  shin_u64 const h = xorf_apply_seed_(hash, header->seed);
  XorfSubHashes_ const subhashes = xorf_make_subhashes_(header, h);
  shin_u16 f = xorf_make_fingerprint_(h);
  for (shin_u8 i = 0; i < 3; i++) {
    f ^= subhashes.h[i];
  }
  return f == 0;
}

static inline shin_u32 xorf_calculate_segment_length_(shin_u32 const size) {
  shin_f64 const sz = (shin_f64)size;
  shin_u32 const base = (uint32_t)floor(log(sz) / log(3.33) + 2.25);
  return 1 << base;
}

static inline shin_f64 xorf_calculate_size_factor_(shin_u32 const size) {
  shin_f64 const sz = (shin_f64)size;
  shin_f64 const result = 0.875 + 0.25 * log(1000000.0) / log(sz);
  return SHIN_MAX(result, ((shin_f64){1.125}));
}

static inline XorfBuilder_ xorf_calculate_builder_(shin_u32 const num_keys) {
  shin_u32 const size = num_keys;
  shin_u32 const segment_length =
      SHIN_MIN(((size == 0) ? 4 : xorf_calculate_segment_length_(size)),
               (shin_u32){262144wbu});
  shin_f64 const size_factor =
      (size <= 1) ? 0 : xorf_calculate_size_factor_(size);
  shin_u32 const capacity =
      (size <= 1) ? 0 : (shin_u32)(round((shin_f64)size) * size_factor);
  shin_u32 const init_segment_count =
      (capacity + segment_length - 1) / segment_length;
  shin_u32 const array_length_calc = init_segment_count * segment_length;
  shin_u32 const segment_count_calc =
      (array_length_calc + segment_length - 1) / segment_length;
  shin_u32 const segment_count =
      (segment_count_calc <= 2) ? 1 : (segment_count_calc - 2);
  shin_u32 const array_length = (segment_count + 2) * segment_length;
  shin_u32 const segment_count_length = segment_count * segment_length;

  return (XorfBuilder_){
      .seed = 0,
      .size = size,
      .segment_length = segment_length,
      .segment_count_length = segment_count_length,
      .segment_count = segment_count,
      .array_length = array_length,
  };
}

static inline ShinStatus xorf_construct(ShinAlloc *alloc,
                                        ShinErrStack *err_stack) {}

#endif

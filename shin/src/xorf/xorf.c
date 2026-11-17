#include <math.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_xoshiro256.h>
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

static inline XorfHeader xorf_builder_to_header_(XorfBuilder_ const *builder) {
  return (XorfHeader){
      .seed = builder->seed,
      .segment_length = builder->segment_length,
      .segment_count_length = builder->segment_count_length,
      .array_length = builder->array_length,
  };
}

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
                          shin_u64 const *restrict const keys,
                          shin_u32 const num_keys, XorfHeader *header_p,
                          shin_u16 **fingerprints_p, ShinErrStack *err_stack) {
  XorfBuilder_ builder = xorf_calculate_builder_(num_keys);

  shin_u32 const max_array_len = builder.array_length;

  shin_u16 *fingerprints = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, builder.array_length * sizeof(shin_u16),
                                (void **)&fingerprints, err_stack)),
               err_stack);

  shin_u64 *set_xormask = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(scratch_alloc, max_array_len * sizeof(shin_u64),
                                (void **)&set_xormask, err_stack)),
               err_stack);

  shin_u32 *set_count = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(scratch_alloc, max_array_len * sizeof(shin_u32),
                                (void **)&set_count, err_stack)),
               err_stack);

  shin_u32 *queue = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(scratch_alloc, max_array_len * sizeof(shin_u32),
                                (void **)&queue, err_stack)),
               err_stack);

  shin_u64 *stack_h = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(scratch_alloc, max_array_len * sizeof(shin_u64),
                                (void **)&stack_h, err_stack)),
               err_stack);

  shin_u8 *stack_hi = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(scratch_alloc, max_array_len * sizeof(shin_u8),
                                (void **)&stack_hi, err_stack)),
               err_stack);

  ShinXoshiro256 rand = ShinXoshiro256_init(0);

  for (shin_usize iter_no = 0; iter_no + 1wbu <= 100wbu; iter_no += 1wbu) {
    shin_u64 const next_seed = ShinXoshiro256_next(&rand);
    builder.seed = next_seed;
    XorfHeader const header = xorf_builder_to_header_(&builder);

    shin_u32 const array_len = builder.array_length;

    shin_u32 stack_len = 0wbu;
    shin_u32 queue_len = 0wbu;

    memset(set_xormask, 0, array_len * sizeof(shin_u64));
    memset(set_count, 0, array_len * sizeof(shin_u32));

    for (shin_u32 i = 0; i < num_keys; i++) {
      shin_u64 const hash = keys[i];
      shin_u64 const h = xorf_apply_seed_(hash, header.seed);
      XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);
      for (shin_u8 i = 0; i < 3; i++) {
        shin_u64 const subh = subhashes.h[i];
        set_xormask[subh] ^= h;
        set_count[subh] += 1wbu;
      }
    }

    for (shin_u32 i = 0; i < array_len; i++) {
      shin_u32 const count = set_count[i];
      if (count == 1) {
        queue[queue_len] = i;
        queue_len += 1wbu;
      }
    }

    while (queue_len > 0) {
      queue_len -= 1wbu;
      shin_u32 const i = queue[queue_len];
      if (set_count[i] == 1) {
        shin_u64 const h = set_xormask[i];
        XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);
        stack_h[stack_len] = h;
        for (shin_u8 hi = 0; hi < 3; hi++) {
          shin_u32 const subh = subhashes.h[hi];
          set_xormask[subh] ^= h;
          set_count[subh] -= 1wbu;
          if (subh == i) {
            stack_hi[stack_len] = hi;
          } else if (set_count[subh] == 1) {
            queue[queue_len] = subh;
            queue_len += 1wbu;
          }
        }
        stack_len += 1wbu;
      }
    }

    if (stack_len < num_keys) {
      continue; // retry
    }

    memset(fingerprints, 0, header.array_length * sizeof(shin_u16));

    while (stack_len > 0) {
      stack_len -= 1wbu;

      shin_u64 const h = stack_h[stack_len];
      shin_u8 const hi = stack_hi[stack_len];
      XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);

      shin_u16 f = xorf_make_fingerprint_(h);
      shin_u32 to_change = 0;

      for (shin_u8 shi = 0; shi < 3; shi++) {
        shin_u32 const subh = subhashes.h[shi];

        if (shi == hi) {
          to_change = subh;
        } else {
          f ^= fingerprints[subh];
        }
      }
      fingerprints[to_change] = f;
    }

    *header_p = header;
    *fingerprints_p = fingerprints;
    return ShinStatus_Ok;
  }

  *fingerprints_p = nullptr;
  SHIN_ERR_RETURN(ShinStatus_UnexpectedError, err_stack);
}

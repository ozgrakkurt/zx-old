#include <xorf/xorf.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_flt.h>
#include <zx/zx_int.h>
#include <zx/zx_xoshiro256.h>

typedef struct XorfBuilder_ {
  zx_u64 seed;
  zx_u32 size;
  zx_u32 segment_length;
  zx_u32 segment_length_mask;
  zx_u32 segment_count;
  zx_u32 segment_count_length;
  zx_u32 array_length;
} XorfBuilder_;

static inline XorfHeader xorf_builder_to_header_(XorfBuilder_ const *builder) {
  return (XorfHeader){
      .seed = builder->seed,
      .segment_length = builder->segment_length,
      .segment_count_length = builder->segment_count_length,
      .array_length = builder->array_length,
  };
}

static inline zx_u32 xorf_calculate_segment_length_(zx_u32 const size) {
  zx_f64 const sz = (zx_f64)size;
  zx_u32 const base = (zx_u32)__builtin_floor(
      __builtin_elementwise_log(sz) / __builtin_elementwise_log(3.33) + 2.25);
  return (zx_u32){1wbu} << base;
}

static inline zx_f64 xorf_calculate_size_factor_(zx_u32 const size) {
  zx_f64 const sz = (zx_f64)size;
  zx_f64 const result = 0.875 + 0.25 * __builtin_elementwise_log(1000000.0) /
                                    __builtin_elementwise_log(sz);
  return zx_flt_max_f64(result, ((zx_f64){1.125}));
}

static inline XorfBuilder_ xorf_calculate_builder_(zx_u32 const num_keys) {
  zx_u32 const size = num_keys;
  zx_u32 const segment_length = zx_int_min_u32(
      ((size == 0) ? (zx_u32){4wbu} : xorf_calculate_segment_length_(size)),
      (zx_u32){262144wbu});
  zx_f64 const size_factor =
      (size <= 1) ? (zx_u32){0wbu} : xorf_calculate_size_factor_(size);
  zx_u32 const capacity =
      (size <= 1) ? (zx_u32){0wbu}
                  : (zx_u32)(__builtin_round((zx_f64)size) * size_factor);
  zx_u32 const init_segment_count =
      (capacity + segment_length - 1wbu) / segment_length;
  zx_u32 const array_length_calc = init_segment_count * segment_length;
  zx_u32 const segment_count_calc =
      (array_length_calc + segment_length - 1wbu) / segment_length;
  zx_u32 const segment_count =
      (segment_count_calc <= 2wbu) ? 1wbu : (segment_count_calc - 2wbu);
  zx_u32 const array_length = (segment_count + 2wbu) * segment_length;
  zx_u32 const segment_count_length = segment_count * segment_length;

  return (XorfBuilder_){
      .seed = 0wbu,
      .size = size,
      .segment_length = segment_length,
      .segment_count_length = segment_count_length,
      .segment_count = segment_count,
      .array_length = array_length,
  };
}

ZXStatus xorf_construct(ZXAlloc *alloc, ZXAlloc *scratch_alloc,
                        zx_u64 const *restrict const keys,
                        zx_u32 const num_keys, XorfHeader *header_p,
                        zx_u16 **fingerprints_p, ZXErrStack *err_stack) {
  XorfBuilder_ builder = xorf_calculate_builder_(num_keys);

  zx_u32 const max_array_len = builder.array_length;

  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, builder.array_length * sizeof(zx_u16),
                            (void **)&fingerprints, err_stack)),
             err_stack);

  zx_u64 *set_xormask = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(scratch_alloc, max_array_len * sizeof(zx_u64),
                            (void **)&set_xormask, err_stack)),
             err_stack);

  zx_u32 *set_count = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(scratch_alloc, max_array_len * sizeof(zx_u32),
                            (void **)&set_count, err_stack)),
             err_stack);

  zx_u32 *queue = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(scratch_alloc, max_array_len * sizeof(zx_u32),
                            (void **)&queue, err_stack)),
             err_stack);

  zx_u64 *stack_h = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(scratch_alloc, max_array_len * sizeof(zx_u64),
                            (void **)&stack_h, err_stack)),
             err_stack);

  zx_u8 *stack_hi = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(scratch_alloc, max_array_len * sizeof(zx_u8),
                            (void **)&stack_hi, err_stack)),
             err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);

  for (zx_usize iter_no = 0; iter_no + 1wbu <= 100wbu; iter_no += 1wbu) {
    zx_u64 const next_seed = ZXXoshiro256_next(&rand);
    builder.seed = next_seed;
    XorfHeader const header = xorf_builder_to_header_(&builder);

    zx_u32 const array_len = builder.array_length;

    zx_u32 stack_len = 0wbu;
    zx_u32 queue_len = 0wbu;

    __builtin_memset(set_xormask, 0, array_len * sizeof(zx_u64));
    __builtin_memset(set_count, 0, array_len * sizeof(zx_u32));

    for (zx_u32 i = 0; i < num_keys; i++) {
      zx_u64 const hash = keys[i];
      zx_u64 const h = xorf_apply_seed_(hash, header.seed);
      XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);
      for (zx_u8 i = 0; i < 3; i++) {
        zx_u64 const subh = subhashes.h[i];
        set_xormask[subh] ^= h;
        set_count[subh] += 1wbu;
      }
    }

    for (zx_u32 i = 0; i < array_len; i++) {
      zx_u32 const count = set_count[i];
      if (count == 1) {
        queue[queue_len] = i;
        queue_len += 1wbu;
      }
    }

    while (queue_len > 0) {
      queue_len -= 1wbu;
      zx_u32 const i = queue[queue_len];
      if (set_count[i] == 1) {
        zx_u64 const h = set_xormask[i];
        XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);
        stack_h[stack_len] = h;
        for (zx_u8 hi = 0; hi < 3; hi++) {
          zx_u32 const subh = subhashes.h[hi];
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

    __builtin_memset(fingerprints, 0, header.array_length * sizeof(zx_u16));

    while (stack_len > 0) {
      stack_len -= 1wbu;

      zx_u64 const h = stack_h[stack_len];
      zx_u8 const hi = stack_hi[stack_len];
      XorfSubHashes_ const subhashes = xorf_make_subhashes_(&header, h);

      zx_u16 f = xorf_make_fingerprint_(h);
      zx_u32 to_change = 0;

      for (zx_u8 shi = 0; shi < 3; shi++) {
        zx_u32 const subh = subhashes.h[shi];

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
    return ZXStatus_Ok;
  }

  *fingerprints_p = nullptr;
  ZX_ERR_RETURN(ZXStatus_UnexpectedError, err_stack);
}

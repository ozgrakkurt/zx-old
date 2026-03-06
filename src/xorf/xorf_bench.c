#include <xorf/xorf.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_usize MEMORY_SIZE = 1 << 30;
constexpr zx_u32 LEN = 1 << 18;

typedef zx_u256 key_type;

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  zx_u64 const *ap = (zx_u64 const *)a;
  zx_u64 const *bp = (zx_u64 const *)b;
  return *ap == *bp;
}

static ZXStatus benchy(ZXAlloc *alloc, ZXErrStack *err_stack) {
  zx_u32 const num_keys = LEN;

  key_type *keys = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, sizeof(key_type) * num_keys, (void **)&keys,
                            err_stack)),
             err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);
  ZXXoshiro256_fill(&rand, (zx_u8 *)keys, sizeof(key_type) * num_keys);

  ZXHashtable hashtable;
  ZX_ERR_TRY((ZXHashtable_init(alloc, num_keys, alignof(zx_u64), sizeof(zx_u64),
                               &hashtable, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, num_keys * sizeof(zx_u64), (void **)&hashes,
                            err_stack)),
             err_stack);

  ZXTimer timer = ZXTimer_init();

  zx_u32 num_hashes = 0;
  for (zx_u32 i = 0; i < num_keys; i++) {
    key_type const key = keys[i];
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    bool const existed_before = ZXHashtable_insert(
        &hashtable, hash, &hash, sizeof(zx_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  fprintf(stderr, "DeDup in %" PRId64 " ticks\n",
          (uint64_t){ZXTimer_lap(&timer)});

  XorfHeader header = {0};
  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                             &fingerprints, err_stack)),
             err_stack);

  fprintf(stderr, "Construct in %" PRId64 " ticks\n",
          (uint64_t){ZXTimer_lap(&timer)});

  for (zx_usize i = 0; i < num_hashes; i++) {
    zx_u64 const hash = hashes[i];
    ZX_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  zx_f64 const query_ticks_per_elem =
      (zx_f64)ZXTimer_lap(&timer) / (zx_f64)num_hashes;
  fprintf(stderr, "Query: %.3f ticks per element\n",
          (double){query_ticks_per_elem});

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_thp_2mb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();

  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  for (zx_u8 i = 0; i < 5; i++) {
    ZX_ERR_TRACE_ABORT((benchy(&alloc, &err_stack)), &err_stack);
    ZX_ERR_TRACE_ABORT((ZXAlloc_reset(&alloc, &err_stack)), &err_stack);
  }

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

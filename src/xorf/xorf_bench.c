#include <inttypes.h>
#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3>
#include <shin/shin_time.h>
#include <shin/shin_xoshiro256.h>
#include <stdio.h>
#include <string.h>
#include <xorf/xorf.h>

constexpr shin_usize MEMORY_SIZE = 1 << 30;
constexpr shin_u32 LEN = 1 << 18;

typedef shin_u256 key_type;

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  shin_u64 const *ap = (shin_u64 const *)a;
  shin_u64 const *bp = (shin_u64 const *)b;
  return *ap == *bp;
}

static ShinStatus benchy(ShinAlloc *alloc, ShinErrStack *err_stack) {
  shin_u32 const num_keys = LEN;

  key_type *keys = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(key_type) * num_keys,
                                (void **)&keys, err_stack)),
               err_stack);

  ShinXoshiro256 rand = ShinXoshiro256_init(0);
  ShinXoshiro256_fill(&rand, (shin_u8 *)keys, sizeof(key_type) * num_keys);

  ShinHashtable hashtable;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, num_keys, alignof(shin_u64),
                                   sizeof(shin_u64), &hashtable, err_stack)),
               err_stack);

  shin_u64 *hashes = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, num_keys * sizeof(shin_u64),
                                (void **)&hashes, err_stack)),
               err_stack);

  ShinTimer timer = ShinTimer_init();

  shin_u32 num_hashes = 0;
  for (shin_u32 i = 0; i < num_keys; i++) {
    key_type const key = keys[i];
    shin_u64 const hash =
        shin_rapidhash_v3((shin_u8 const *)&key, sizeof(key_type), 0);

    bool const existed_before = ShinHashtable_insert(
        &hashtable, hash, &hash, sizeof(shin_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  fprintf(stderr, "DeDup in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  XorfHeader header = {0};
  shin_u16 *fingerprints = nullptr;
  SHIN_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                               &fingerprints, err_stack)),
               err_stack);

  fprintf(stderr, "Construct in %" PRId64 " ticks\n",
          (uint64_t){ShinTimer_lap(&timer)});

  for (shin_usize i = 0; i < num_hashes; i++) {
    shin_u64 const hash = hashes[i];
    SHIN_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  shin_f64 const query_ticks_per_elem =
      (shin_f64)ShinTimer_lap(&timer) / (shin_f64)num_hashes;
  fprintf(stderr, "Query: %.3f ticks per element\n",
          (double){query_ticks_per_elem});

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_thp_2mb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();

  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  for (shin_u8 i = 0; i < 5; i++) {
    SHIN_ERR_TRACE_ABORT((benchy(&alloc, &err_stack)), &err_stack);
    SHIN_ERR_TRACE_ABORT((ShinAlloc_reset(&alloc, &err_stack)), &err_stack);
  }

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

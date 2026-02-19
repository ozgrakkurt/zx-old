#include <shin/shin_alloc.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>
#include <shin/shin_time.h>
#include <shin/shin_xoshiro256.h>
#include <stdint.h>
#include <stdio.h>

constexpr shin_usize MEMORY_SIZE = 1 << 30;
constexpr shin_usize CAP = 40000;

typedef shin_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

static inline ShinStatus do_inserts(ShinAlloc *alloc, ShinErrStack *err_stack) {
  ShinHashtable table;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, CAP, alignof(key_type),
                                   sizeof(key_type), &table, err_stack)),
               err_stack);

  shin_u64 *hashes = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(shin_u64) * CAP, (void **)&hashes,
                                err_stack)),
               err_stack);
  SHIN_ASSERT((hashes != nullptr));

  for (shin_usize x = 0; x < 5; x++) {
    ShinTimer timer = ShinTimer_init();

    ShinXoshiro256 rand = ShinXoshiro256_init(0);
    for (shin_usize i = 0; i < CAP; i++) {
      key_type key = 0;
      ShinXoshiro256_fill(&rand, (shin_u8 *)&key, sizeof(key_type));

      shin_u64 const hash =
          shin_rapidhash_v3((shin_u8 const *)&key, sizeof(key_type), 0);
      hashes[i] = hash;
    }

    shin_f64 const hash_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Hashing: %.3f ticks per element\n", (double){hash_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      ShinHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                           keys_eql);
    }

    shin_f64 const insert_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Insert: %.3f ticks per element\n", (double){insert_ticks_per_elem});

    for (shin_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      SHIN_ASSERT(
          (nullptr != ShinHashtable_get(&table, hashes[i], &key,
                                        sizeof(key_type), nullptr, keys_eql)));
    }

    shin_f64 const get_ticks_per_elem =
        (shin_f64)ShinTimer_lap(&timer) / (shin_f64)CAP;
    printf("Get: %.3f ticks per element\n", (double){get_ticks_per_elem});
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_thp_2mb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((do_inserts(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

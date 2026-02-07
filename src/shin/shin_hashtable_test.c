#include <shin/shin_alloc.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>

constexpr shin_usize MEMORY_SIZE = 1 << 20;
constexpr shin_usize CAP = 3169;

typedef shin_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

static inline ShinStatus run_test(ShinAlloc *alloc, ShinErrStack *err_stack) {
  ShinHashtable table;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, CAP, alignof(key_type),
                                   sizeof(key_type), &table, err_stack)),
               err_stack);

  for (shin_usize i = 0; i < CAP; i++) {
    key_type const key = i;
    shin_u64 const hash =
        shin_rapidhash_v3((shin_u8 const *)&key, sizeof(key_type), 0);

    SHIN_ERR_ASSERT((ShinHashtable_insert(&table, hash, &key, sizeof(key_type),
                                          nullptr, keys_eql) == false),
                    err_stack);
  }

  for (shin_usize i = 0; i < CAP; i++) {
    key_type const key = i;
    shin_u64 const hash =
        shin_rapidhash_v3((shin_u8 const *)&key, sizeof(key_type), 0);

    void const *entry = ShinHashtable_get(&table, hash, &key, sizeof(key_type),
                                          nullptr, keys_eql);
    SHIN_ERR_ASSERT((entry != nullptr), err_stack);

    key_type const *entry_key_ptr = (key_type const *)entry;
    SHIN_ERR_ASSERT(((*entry_key_ptr) == key), err_stack);
  }

  return ShinStatus_Ok;
}

int main(void) {
  void *const page = shin_page_alloc_4kb(MEMORY_SIZE);

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(page, MEMORY_SIZE);

  SHIN_ERR_TRACE_ABORT((run_test(&alloc, &err_stack)), &err_stack);

  shin_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

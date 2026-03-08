#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

static inline ZXStatus smoke_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXHashtable table;
  ZX_ERR_TRY((ZXHashtable_init(alloc, CAP, (zx_u16)alignof(key_type),
                               sizeof(key_type), &table, err_stack)),
             err_stack);

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    ZX_ERR_ASSERT((ZXHashtable_insert(&table, hash, &key, sizeof(key_type),
                                      nullptr, keys_eql) == false),
                  err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    void const *entry = ZXHashtable_get(&table, hash, &key, sizeof(key_type),
                                        nullptr, keys_eql);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);

    key_type const *entry_key_ptr = (key_type const *)entry;
    ZX_ERR_ASSERT(((*entry_key_ptr) == key), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    void const *entry = ZXHashtable_get(&table, hash, &key, sizeof(key_type),
                                        nullptr, keys_eql);
    ZX_ERR_ASSERT((entry == nullptr), err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    void const *entry = ZXHashtable_get_or_insert(
        &table, hash, &key, sizeof(key_type), nullptr, keys_eql);
    ZX_ERR_ASSERT((ZXHashtable_capacity_left(&table) == (CAP - CAP / 2)),
                  err_stack);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);

    key_type const *entry_key_ptr = (key_type const *)entry;
    ZX_ERR_ASSERT(((*entry_key_ptr) == key), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    void const *entry = ZXHashtable_get_or_insert(
        &table, hash, &key, sizeof(key_type), nullptr, keys_eql);
    ZX_ERR_ASSERT((ZXHashtable_capacity_left(&table) == (CAP - i - 1)),
                  err_stack);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);

    key_type const *entry_key_ptr = (key_type const *)entry;
    ZX_ERR_ASSERT(((*entry_key_ptr) == key), err_stack);
  }

  return ZXStatus_Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXHashtable table;
  ZX_ERR_TRY((ZXHashtable_init(alloc, 0, (zx_u16)alignof(key_type),
                               sizeof(key_type), &table, err_stack)),
             err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_4kb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  ZX_ERR_TRACE_TRY_ABORT((smoke_test(&alloc, &err_stack)), &err_stack);
  ZXAlloc_reset(&alloc);

  ZX_ERR_TRACE_TRY_ABORT((zero_size_test(&alloc, &err_stack)), &err_stack);

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

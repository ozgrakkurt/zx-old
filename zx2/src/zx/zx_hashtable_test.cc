#include <zx/zx_alloc.h>
#include <zx/zx_debug_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 key_type;

static inline bool keys_eql(void const *ctx, key_type const *a,
                            key_type const *b) {
  (void)ctx;
  return *a == *b;
}

using HTable = ZXHashtable<key_type, void const *, keys_eql>;

static inline ZXStatus smoke_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  HTable table;
  ZX_ERR_TRY((HTable::init(alloc, CAP, &table, err_stack)), err_stack);

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    ZX_ERR_ASSERT((table.insert(hash, &key, nullptr) == false), err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    key_type const *entry = table.get(hash, &key, nullptr);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);
    ZX_ERR_ASSERT(((*entry) == key), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    key_type const *entry = table.get(hash, &key, nullptr);
    ZX_ERR_ASSERT((entry == nullptr), err_stack);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    key_type const *entry = table.get_or_insert(hash, &key, nullptr);
    ZX_ERR_ASSERT((table.capacity_left() == (CAP - CAP / 2)), err_stack);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);
    ZX_ERR_ASSERT((*entry == key), err_stack);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    key_type const *entry = table.get_or_insert(hash, &key, nullptr);
    ZX_ERR_ASSERT((table.capacity_left() == (CAP - i - 1)), err_stack);
    ZX_ERR_ASSERT((entry != nullptr), err_stack);
    ZX_ERR_ASSERT((*entry == key), err_stack);
  }

  return ZXStatus::Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  HTable table;
  ZX_ERR_TRY((HTable::init(alloc, 0, &table, err_stack)), err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL((ZXDebugAlloc::init(MEMORY_SIZE, &dbg_alloc, &err_stack)),
                   &err_stack);
  ZXAlloc alloc = dbg_alloc.make_alloc();

  ZX_ERR_TRY_FATAL((smoke_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((zero_size_test(alloc, &err_stack)), &err_stack);

  return 0;
}

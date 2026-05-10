#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

typedef zx_u256 key_type;

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static bool keys_eql(void const *ctx, key_type const *a, key_type const *b) {
  (void)ctx;
  return *a == *b;
}

static zx_u64 hash_key(key_type key) {
  return zx_rapidhash_v3(ZXSlice<zx_u8>::init_from_ptr_unsafe(&key), 0);
}

using HTable = ZXHashtable<key_type, void const *, keys_eql>;

static void smoke_test_inner(HTable *table, zx_u32 len) {
  assert(table->capacity_left() == len);

  for (zx_u32 i = 0; i < zx_div_unsafe_u32(len, 2); i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);
    assert(ZXHashtableInsertStatus::CreatedNewEntry ==
           table->insert(hash, &key, nullptr));
    assert(ZXHashtableInsertStatus::ModifiedExistingEntry ==
           table->insert(hash, &key, nullptr));
  }

  for (zx_u32 i = 0; i < zx_div_unsafe_u32(len, 2); i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const *entry = nullptr;
    assert(ZXHashtableGetStatus::Found ==
           table->get(hash, &key, nullptr, &entry));
    assert(entry != nullptr);
    assert(*entry == key);
  }

  for (zx_u32 i = zx_div_unsafe_u32(len, 2); i < len; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const *entry = nullptr;
    assert(ZXHashtableGetStatus::NotFound ==
           table->get(hash, &key, nullptr, &entry));
    assert(entry == nullptr);
  }

  for (zx_u32 i = 0; i < zx_div_unsafe_u32(len, 2); i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const *entry = nullptr;
    assert(ZXHashtableGetOrInsertStatus::FoundExistingEntry ==
           table->get_or_insert(hash, &key, nullptr, &entry));
    assert(table->capacity_left() ==
           zx_sub_unsafe_u32(len, zx_div_unsafe_u32(len, 2)));
    assert(entry != nullptr);
    assert(*entry == key);
  }

  for (zx_u32 i = zx_div_unsafe_u32(len, 2); i < len; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const *entry = nullptr;
    assert(ZXHashtableGetOrInsertStatus::CreatedNewEntry ==
           table->get_or_insert(hash, &key, nullptr, &entry));
    assert(table->capacity_left() ==
           zx_sub_unsafe_u32(len, zx_add_unsafe_u32(i, 1)));
    assert(entry != nullptr);
    assert(*entry == key);
  }
}

static void smoke_test(ZXAlloc *alloc, zx_u32 len) {
  ZXSliceMut<zx_u8> mem;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(1 << 16, &mem));
  ZXFmt fmt = ZXFmt::init(mem);

  HTable table;
  ZXHashtableInitStatus const init_status = HTable::init(alloc, len, &table);
  if (ZXHashtableInitStatus::Ok != init_status) {
    bool fmt_overflow = false;
    fmt.push_cstring(zx_hashtable_init_status_to_cstring(init_status),
                     &fmt_overflow);
    fmt.push_char('\n', &fmt_overflow);
    (void)fmt_overflow;
    zx_u64 n_written = 0;
    ZXSysStatus const w_status =
        zx_sys_write(ZX_STDERR, fmt.written(), &n_written);
    (void)w_status;
    (void)n_written;
    __builtin_trap();
  }

  smoke_test_inner(&table, len);
  table.reset();
  smoke_test_inner(&table, len);
}

static void nocap_test(ZXAlloc *alloc) {
  HTable table;
  assert(ZXHashtableInitStatus::Ok == HTable::init(alloc, 0, &table));
  assert(table.capacity_left() == 0);

  key_type const key = 100;
  zx_u64 const hash = hash_key(key);

  key_type const *entry_p = nullptr;

  assert(ZXHashtableGetStatus::NotFound ==
         table.get(hash, &key, nullptr, &entry_p));
  assert(entry_p == nullptr);
  assert(ZXHashtableInsertStatus::OutOfCapacity ==
         table.insert(hash, &key, nullptr));

  assert(ZXHashtableGetOrInsertStatus::OutOfCapacity ==
         table.get_or_insert(hash, &key, nullptr, &entry_p));
  assert(entry_p == nullptr);

  assert(ZXHashtableGetStatus::NotFound ==
         table.get(hash, &key, nullptr, &entry_p));
  assert(entry_p == nullptr);
}

static void noalloc_test() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());
  HTable table;
  assert(ZXHashtableInitStatus::OutOfMemory ==
         HTable::init(&noalloc, 0, &table));
  assert(ZXHashtableInitStatus::OutOfMemory ==
         HTable::init(&noalloc, 1, &table));
  assert(ZXHashtableInitStatus::OutOfMemory ==
         HTable::init(&noalloc, 10, &table));
  assert(ZXHashtableInitStatus::OutOfMemory ==
         HTable::init(&noalloc, 1000000, &table));
  assert(ZXHashtableInitStatus::IntegerOverflow ==
         HTable::init(&noalloc, ZX_U32_MAX, &table));
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = 1 << 20;

  ZXSliceMut<zx_u8> mem;
  assert(zx_page_alloc_4kb(MEMORY_SIZE, &mem) == ZXPageAllocStatus::Ok);

  noalloc_test();

  ZXAlloc alloc = ZXAlloc::init(mem);

  nocap_test(&alloc);
  alloc.reset();

  constexpr zx_u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129,  3169};
  constexpr zx_u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u32 i = 0; i < n_lengths; i++) {
    zx_u32 const len = lengths[i];
    alloc.reset();
    smoke_test(&alloc, len);
  }

  assert(zx_page_free(mem) == ZXPageAllocStatus::Ok);
  return 0;
}

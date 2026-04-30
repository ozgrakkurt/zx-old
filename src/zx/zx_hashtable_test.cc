#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 key_type;

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static bool keys_eql(void const* ctx, key_type const* a, key_type const* b) {
  (void)ctx;
  return *a == *b;
}

static zx_u64 hash_key(key_type key) {
  return zx_rapidhash_v3(ZXSlice<zx_u8>::init_from_ptr_unsafe(&key), 0);
}

using HTable = ZXHashtable<key_type, void const*, keys_eql>;

static void smoke_test(ZXAlloc* alloc) {
  ZXSliceMut<zx_u8> mem;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(1 << 16, &mem));
  ZXFmt fmt = ZXFmt::init(mem);

  HTable table;
  ZXHashtableInitStatus const init_status = HTable::init(alloc, CAP, &table);
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

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);
    assert(ZXHashtableInsertStatus::CreatedNewEntry ==
           table.insert(hash, &key, nullptr));
    assert(ZXHashtableInsertStatus::ModifiedExistingEntry ==
           table.insert(hash, &key, nullptr));
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const* entry = nullptr;
    assert(ZXHashtableGetStatus::Found ==
           table.get(hash, &key, nullptr, &entry));
    assert(entry != nullptr);
    assert(*entry == key);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const* entry = nullptr;
    assert(ZXHashtableGetStatus::NotFound ==
           table.get(hash, &key, nullptr, &entry));
    assert(entry == nullptr);
  }

  for (zx_u64 i = 0; i < CAP / 2; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const* entry = nullptr;
    assert(ZXHashtableGetOrInsertStatus::FoundExistingEntry ==
           table.get_or_insert(hash, &key, nullptr, &entry));
    assert(table.capacity_left() ==
           zx_sub_unsafe_u32(CAP, zx_div_unsafe_u32(CAP, 2)));
    assert(entry != nullptr);
    assert(*entry == key);
  }

  for (zx_u64 i = CAP / 2; i < CAP; i++) {
    key_type const key = i;
    zx_u64 const hash = hash_key(key);

    key_type const* entry = nullptr;
    assert(ZXHashtableGetOrInsertStatus::CreatedNewEntry ==
           table.get_or_insert(hash, &key, nullptr, &entry));
    assert(table.capacity_left() ==
           zx_sub_unsafe_u64(CAP, zx_add_unsafe_u64(i, 1)));
    assert(entry != nullptr);
    assert(*entry == key);
  }
}

static void nocap_test(ZXAlloc* alloc) {
  HTable table;
  assert(ZXHashtableInitStatus::Ok == HTable::init(alloc, 0, &table));
  assert(table.capacity_left() == 0);

  key_type const key = 100;
  zx_u64 const hash = hash_key(key);

  key_type const* entry_p = nullptr;

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
  ZXSliceMut<zx_u8> mem;
  assert(zx_page_alloc_4kb(MEMORY_SIZE, &mem) == ZXPageAllocStatus::Ok);

  noalloc_test();

  ZXAlloc alloc = ZXAlloc::init(mem);
  nocap_test(&alloc);
  alloc.reset();
  smoke_test(&alloc);

  assert(zx_page_free(mem) == ZXPageAllocStatus::Ok);
  return 0;
}

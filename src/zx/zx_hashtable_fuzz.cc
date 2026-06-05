#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_panic.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_slice.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    zx_panic();
  }
}

struct Ctx {
  ZXAlloc alloc;
};

typedef zx_u16 key_type;
typedef zx_i256 val_type;

struct KeyVal {
  val_type val;
  key_type key;
};

struct RefMap {
  ZXSliceMut<KeyVal> m_entries;
  zx_u32 m_offset;

  static inline RefMap init(ZXAlloc *alloc, zx_u32 capacity) {
    ZXSliceMut<KeyVal> entries;
    assert(ZXAllocStatus::Ok == alloc->alloc<KeyVal>(capacity, &entries));
    return RefMap{
        .m_entries = entries,
        .m_offset = 0,
    };
  }

  inline void reset() {
    this->m_entries.memset(0xaa);
    this->m_offset = 0;
  }

  inline zx_u32 len() const { return this->m_offset; }

  inline zx_u32 capacity() const {
    return zx_cast_unsafe_to_u32(this->m_entries.len());
  }

  inline zx_u32 capacity_left() const {
    return zx_sub_unsafe_u32(this->capacity(), this->m_offset);
  }

  inline KeyVal select_random(ZXXoshiro256 *rand) const {
    assert(this->len() > 0);
    zx_u32 const idx = zx_mod_unsafe_u32((zx_u32)rand->next(), this->len());
    return this->m_entries.read_unsafe(idx);
  }

  inline bool insert(KeyVal kv) {
    for (zx_u32 i = 0; i < this->m_offset; i++) {
      KeyVal *entry = this->m_entries.offset_unsafe(i);
      if (entry->key == kv.key) {
        entry->val = kv.val;
        return true;
      }
    }
    if (this->capacity_left() == 0) {
      return false;
    }
    this->m_entries.write_unsafe(this->m_offset, kv);
    this->m_offset = zx_add_unsafe_u32(this->m_offset, 1);
    return true;
  }

  inline bool remove(key_type key) {
    for (zx_u32 i = 0; i < this->m_offset; i++) {
      KeyVal *entry = this->m_entries.offset_unsafe(i);
      if (entry->key == key) {
        *entry = this->m_entries.read_unsafe(this->m_offset);
        this->m_offset = zx_sub_unsafe_u32(this->m_offset, 1);
        return true;
      }
    }

    return false;
  }

  inline bool get(key_type key, KeyVal const **out) const {
    assert(out != nullptr);
    for (zx_u32 i = 0; i < this->len(); i++) {
      KeyVal const *entry = this->m_entries.offset_unsafe(i);
      if (entry->key == key) {
        *out = entry;
        return true;
      }
    }
    *out = nullptr;
    return false;
  }

  inline bool get_mut(key_type key, KeyVal **out) const {
    assert(out != nullptr);
    for (zx_u32 i = 0; i < this->len(); i++) {
      KeyVal *entry = this->m_entries.offset_unsafe(i);
      if (entry->key == key) {
        *out = entry;
        return true;
      }
    }
    *out = nullptr;
    return false;
  }
};

static bool keys_eql(void const *ctx, KeyVal const *l, KeyVal const *r) {
  (void)ctx;
  return l->key == r->key;
}

static zx_u64 make_hash(key_type key) {
  return zx_rapidhash_v3(ZXSlice<zx_u8>::init_from_ptr_unsafe(&key), 169);
}

using HTable = ZXHashtable<KeyVal, void const *, keys_eql>;

static void fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXAlloc *alloc = &((Ctx *)usr_ctx)->alloc;
  alloc->reset();

  constexpr zx_u32 max_capacity = 128;
  zx_u32 const capacity = zx_cast_unsafe_to_u32<zx_u64>(
      zx_mod_unsafe_u64(fuzz_input->next(), max_capacity));

  constexpr zx_u32 max_n_ops = zx_mul_unsafe_u32(max_capacity, 4);
  zx_u32 const n_ops = zx_cast_unsafe_to_u32<zx_u64>(
      zx_mod_unsafe_u64(fuzz_input->next(), max_n_ops));

  HTable table;
  assert(ZXHashtableInitStatus::Ok == HTable::init(alloc, capacity, &table));

  RefMap map = RefMap::init(alloc, capacity);

  ZXXoshiro256 val_rand = ZXXoshiro256::init(fuzz_input->next());

  for (zx_u32 i = 0; i < n_ops; i++) {
    assert(table.capacity_left() == map.capacity_left());
    assert(table.capacity() == map.capacity());
    assert(table.n_occupied() == map.len());

    zx_u8 const op =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(fuzz_input->next(), 4));
    zx_u64 const seed = fuzz_input->next();
    ZXXoshiro256 key_rand = ZXXoshiro256::init(seed);
    switch (op) {
    // insert
    case 0: {
      KeyVal const kv = KeyVal{
          .key = key_rand.random<key_type>(),
          .val = val_rand.random<val_type>(),
      };
      zx_u64 const hash = make_hash(kv.key);

      ZXHashtableInsertStatus const hstatus = table.insert(hash, &kv, nullptr);
      if (ZXHashtableInsertStatus::CreatedNewEntry == hstatus) {
        KeyVal const *kvp = nullptr;
        assert(!map.get(kv.key, &kvp));
        assert(map.insert(kv));
      } else if (ZXHashtableInsertStatus::ModifiedExistingEntry == hstatus) {
        KeyVal const *kvp = nullptr;
        assert(map.get(kv.key, &kvp));
        assert(kvp != nullptr);
        assert(kvp->key == kv.key);
        assert(map.insert(kv));
      } else if (ZXHashtableInsertStatus::OutOfCapacity == hstatus) {
        assert(!map.insert(kv));
      } else {
        zx_panic();
      }

      if (hstatus != ZXHashtableInsertStatus::OutOfCapacity) {
        KeyVal const *kvp = nullptr;
        assert(ZXHashtableGetStatus::Found ==
               table.get(hash, &kv, nullptr, &kvp));
        assert(kvp != nullptr);
        assert(kvp->key == kv.key && kvp->val == kv.val);
      } else {
        KeyVal const *kvp = nullptr;
        assert(ZXHashtableGetStatus::NotFound ==
               table.get(hash, &kv, nullptr, &kvp));
      }

      break;
    }
    // get_or_insert
    case 1: {
      KeyVal const kv = KeyVal{
          .key = key_rand.random<key_type>(),
          .val = val_rand.random<val_type>(),
      };
      zx_u64 const hash = make_hash(kv.key);

      KeyVal const *kvp = nullptr;
      ZXHashtableGetOrInsertStatus const hstatus =
          table.get_or_insert(hash, &kv, nullptr, &kvp);
      if (ZXHashtableGetOrInsertStatus::CreatedNewEntry == hstatus) {
        KeyVal const *kvp2 = nullptr;
        assert(!map.get(kv.key, &kvp2));
        assert(map.insert(kv));
      } else if (ZXHashtableGetOrInsertStatus::FoundExistingEntry == hstatus) {
        KeyVal const *kvp2 = nullptr;
        assert(map.get(kv.key, &kvp2));
        assert(kvp2 != nullptr);
        assert(kvp2->key == kv.key);
      } else if (ZXHashtableGetOrInsertStatus::OutOfCapacity == hstatus) {
        assert(!map.insert(kv));
      } else {
        zx_panic();
      }

      if (hstatus != ZXHashtableGetOrInsertStatus::OutOfCapacity) {
        KeyVal const *kvp2 = nullptr;
        assert(ZXHashtableGetStatus::Found ==
               table.get(hash, &kv, nullptr, &kvp2));
        assert(kvp2 != nullptr);
        assert(kvp->key == kvp2->key && kvp->val == kvp2->val);
      } else {
        KeyVal const *kvp2 = nullptr;
        assert(ZXHashtableGetStatus::NotFound ==
               table.get(hash, &kv, nullptr, &kvp2));
      }

      break;
    }
    // rand get
    case 2: {
      KeyVal const kv = KeyVal{
          .key = key_rand.random<key_type>(),
          .val = val_rand.random<val_type>(),
      };
      zx_u64 const hash = make_hash(kv.key);
      KeyVal const *kvp = nullptr;
      ZXHashtableGetStatus const hstatus = table.get(hash, &kv, nullptr, &kvp);
      if (hstatus == ZXHashtableGetStatus::Found) {
        assert(kvp != nullptr);
        KeyVal const *expected_kvp = nullptr;
        assert(map.get(kv.key, &expected_kvp));
        assert(expected_kvp != nullptr);
        assert(kvp->key == expected_kvp->key);
        assert(kvp->val == expected_kvp->val);
      } else if (hstatus == ZXHashtableGetStatus::NotFound) {
        KeyVal const *expected_kvp = nullptr;
        assert(!map.get(kv.key, &expected_kvp));
      } else {
        zx_panic();
      }
      break;
    }
    // rand select
    case 3: {
      if (map.len() == 0) {
        continue;
      }

      KeyVal const kv = map.select_random(&key_rand);
      zx_u64 const hash = make_hash(kv.key);
      KeyVal const *kvp_im = nullptr;
      assert(ZXHashtableGetStatus::Found ==
             table.get(hash, &kv, nullptr, &kvp_im));

      assert(kvp_im != nullptr);

      assert(kvp_im->key == kv.key && kvp_im->val == kv.val);
      break;
    }
    default: {
      zx_panic();
      break;
    }
    }
  }
}

int main(void) {
  constexpr zx_u64 mem_cap = zx_u64{1} << 28;
  ZXSliceMut<zx_u8> mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_4kb(mem_cap, &mem));

  ZXAlloc alloc = ZXAlloc::init(mem);

  constexpr zx_u64 fuzz_mem_cap = zx_u64{1} << 27;
  ZXSliceMut<zx_u8> fuzz_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(fuzz_mem_cap, &fuzz_mem));

  ZXAlloc fuzz_alloc = ZXAlloc::init(fuzz_mem);

  constexpr zx_u64 op_mem_cap = zx_sub_unsafe_u64(mem_cap, fuzz_mem_cap);
  ZXSliceMut<zx_u8> op_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(op_mem_cap, &op_mem));

  ZXAlloc op_alloc = ZXAlloc::init(op_mem);
  Ctx ctx = Ctx{
      .alloc = op_alloc,
  };
  zx_fuzz(1 << 18, &fuzz_alloc, (void *)&ctx, fuzz_one);
  return 0;
}

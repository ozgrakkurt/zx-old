#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>
#include <stddef.h>
#include <stdint.h>

typedef shin_u8 key_type;
typedef shin_u8 val_type;

typedef struct KeyVal {
  key_type key;
  val_type val;
} KeyVal;

typedef struct InputEntry {
  // kind of operation to do
  shin_u8 op;
  KeyVal kv;
} InputEntry;

constexpr shin_usize MEM_SIZE = ((shin_usize){1wbu} << 23);
constexpr shin_usize CAPACITY = 319wbu;

_Thread_local void *mem;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  mem = shin_page_alloc_thp_2mb(MEM_SIZE);

  return 0;
}

static inline bool keys_eql(void const *ctx, void const *l, void const *r) {
  (void)ctx;

  key_type const *lk = (key_type const *)l;
  key_type const *rk = (key_type const *)r;

  return *lk == *rk;
}

static inline bool kv_eql(KeyVal l, KeyVal r) {
  return (l.key == r.key) && (l.val == r.val);
}

static inline KeyVal *key_in_ref(KeyVal *ref_data, shin_u32 ref_data_len,
                                 key_type key) {
  for (shin_u32 i = 0; i < ref_data_len; i++) {
    if (ref_data[i].key == key) {
      return &ref_data[i];
    }
  }

  return nullptr;
}

static inline bool ref_insert(KeyVal *ref_data, shin_u32 *ref_data_len,
                              KeyVal kv) {
  shin_u32 const len = *ref_data_len;
  KeyVal *kvp = key_in_ref(ref_data, len, kv.key);

  if (kvp != nullptr) {
    kvp->val = kv.val;
    return true;
  } else {
    ref_data[len] = kv;
    *ref_data_len = len + 1;
    return false;
  }
}

static inline KeyVal const *
ref_get_or_insert(KeyVal *ref_data, shin_u32 *ref_data_len, KeyVal kv) {
  shin_u32 const len = *ref_data_len;
  KeyVal *kvp = key_in_ref(ref_data, len, kv.key);

  if (kvp != nullptr) {
    return kvp;
  } else {
    ref_data[len] = kv;
    *ref_data_len = len + 1;
    return &ref_data[len];
  }
}

static inline KeyVal const *ref_get(KeyVal *ref_data, shin_u32 ref_data_len,
                                    key_type key) {
  return key_in_ref(ref_data, ref_data_len, key);
}

static ShinStatus test_one(ShinAlloc *alloc, InputEntry const *inputs,
                           shin_u32 capacity, ShinErrStack *err_stack) {
  ShinHashtable hashtable;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, capacity, alignof(KeyVal),
                                   sizeof(KeyVal), &hashtable, err_stack)),
               err_stack);

  // define a simple array to be use for differential fuzzing
  KeyVal *ref_data = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, sizeof(KeyVal) * capacity,
                                (void **)&ref_data, err_stack)),
               err_stack);
  SHIN_ERR_ASSERT((ref_data != nullptr), err_stack);
  shin_u32 ref_data_len = 0;

  for (shin_usize i = 0; i < capacity; i++) {
    SHIN_ERR_ASSERT(
        ((CAPACITY - ShinHashtable_capacity_left(&hashtable)) == ref_data_len),
        err_stack);

    InputEntry const in = inputs[i];
    shin_u64 const hash = shin_rapidhash_v3(&in.kv.key, sizeof(key_type), 0);

    switch (in.op % 3) {
    case 0: {
      bool const did_exist = ShinHashtable_insert(
          &hashtable, hash, &in.kv, sizeof(KeyVal), nullptr, keys_eql);

      bool const ref_did_exist = ref_insert(ref_data, &ref_data_len, in.kv);

      SHIN_ERR_ASSERT((did_exist == ref_did_exist), err_stack);

      break;
    }
    case 1: {
      shin_u32 const initial_cap_left = ShinHashtable_capacity_left(&hashtable);
      void const *entry = ShinHashtable_get_or_insert(
          &hashtable, hash, &in.kv, sizeof(KeyVal), nullptr, keys_eql);
      shin_u32 const after_cap_left = ShinHashtable_capacity_left(&hashtable);
      bool const did_exist = initial_cap_left == after_cap_left + 1;

      // has to either exist before ot operation did nothing.
      SHIN_ERR_ASSERT((did_exist || (initial_cap_left == after_cap_left)),
                      err_stack);

      // should always point to a valid entry inside hashtable.
      SHIN_ERR_ASSERT((entry != nullptr), err_stack);

      KeyVal const *ref_entry =
          ref_get_or_insert(ref_data, &ref_data_len, in.kv);
      SHIN_ERR_ASSERT((kv_eql(*ref_entry, *(KeyVal const *)entry)), err_stack);

      break;
    }
    case 2: {
      void const *entry = ShinHashtable_get(&hashtable, hash, &in.kv,
                                            sizeof(KeyVal), nullptr, keys_eql);

      KeyVal const *ref_entry = ref_get(ref_data, ref_data_len, in.kv.key);

      if (ref_entry == nullptr) {
        SHIN_ERR_ASSERT((entry == nullptr), err_stack);
      } else {
        SHIN_ERR_ASSERT((entry != nullptr), err_stack);

        SHIN_ERR_ASSERT((kv_eql(*ref_entry, *(KeyVal const *)entry)),
                        err_stack);
      }

      break;
    }
    default: {
      SHIN_ASSERT(false);
      break;
    }
    }
  }

  return ShinStatus_Ok;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  // check before doing pointer cast
  constexpr shin_usize RATIO = sizeof(InputEntry) / sizeof(shin_u8);
  _Static_assert(alignof(InputEntry) == alignof(shin_u8));

  InputEntry const *inputs = (InputEntry const *)fuzz_input;
  shin_usize const capacity =
      SHIN_MIN(CAPACITY, ((shin_usize){fuzz_input_len} / RATIO));

  ShinAlloc alloc = ShinAlloc_init(mem, MEM_SIZE);
  ShinErrStack err_stack = ShinErrStack_init();

  SHIN_ASSERT(((shin_usize){SHIN_UINT_MAX(shin_u32)} >= capacity));

  SHIN_ERR_TRACE_ABORT(
      (test_one(&alloc, inputs, (shin_u32)capacity, &err_stack)), &err_stack);

  return 0;
}

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>

typedef struct FuzzCtx {
  void *mem;
  zx_u32 cap;
} FuzzCtx;

typedef zx_u32 key_type;
typedef zx_u32 val_type;

typedef struct KeyVal {
  key_type key;
  val_type val;
} KeyVal;

static inline bool keys_eql(void const *ctx, void const *l, void const *r) {
  (void)ctx;

  key_type const *lk = (key_type const *)l;
  key_type const *rk = (key_type const *)r;

  return *lk == *rk;
}

static inline bool kv_eql(KeyVal l, KeyVal r) {
  return (l.key == r.key) && (l.val == r.val);
}

static inline KeyVal *key_in_ref(KeyVal *ref_data, zx_u32 ref_data_len,
                                 key_type key) {
  for (zx_u32 i = 0; i < ref_data_len; i++) {
    if (ref_data[i].key == key) {
      return &ref_data[i];
    }
  }

  return nullptr;
}

static inline bool ref_insert(KeyVal *ref_data, zx_u32 *ref_data_len,
                              KeyVal kv) {
  zx_u32 const len = *ref_data_len;
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

static inline KeyVal const *ref_get_or_insert(KeyVal *ref_data,
                                              zx_u32 *ref_data_len, KeyVal kv) {
  zx_u32 const len = *ref_data_len;
  KeyVal *kvp = key_in_ref(ref_data, len, kv.key);

  if (kvp != nullptr) {
    return kvp;
  } else {
    ref_data[len] = kv;
    *ref_data_len = len + 1;
    return &ref_data[len];
  }
}

static inline KeyVal const *ref_get(KeyVal *ref_data, zx_u32 ref_data_len,
                                    key_type key) {
  return key_in_ref(ref_data, ref_data_len, key);
}

static ZXStatus fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input,
                         ZXErrStack *err_stack) {
  FuzzCtx const *ctx = (FuzzCtx const *)usr_ctx;
  ZXAlloc alloc = ZXAlloc_init(ctx->mem, ctx->cap);

  zx_u64 fuzz_in = 0;
  ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &fuzz_in, err_stack)), err_stack);
  zx_u32 const MAX_CAP = 1024;
  zx_u32 const capacity = ((zx_u32)fuzz_in) % MAX_CAP;

  ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &fuzz_in, err_stack)), err_stack);
  zx_u32 const MAX_N_OPS = 1024;
  zx_u32 const n_ops = ((zx_u32)fuzz_in) % MAX_N_OPS;

  ZXHashtable hashtable;
  ZX_ERR_TRY((ZXHashtable_init(&alloc, capacity, alignof(KeyVal),
                               sizeof(KeyVal), &hashtable, err_stack)),
             err_stack);

  // define a simple array to be used for differential fuzzing
  KeyVal *ref_data = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(&alloc, sizeof(KeyVal) * capacity,
                            (void **)&ref_data, err_stack)),
             err_stack);
  zx_u32 ref_data_len = 0;

  for (zx_u32 i = 0; i < n_ops; i++) {
    ZX_ERR_ASSERT(
        ((capacity - ZXHashtable_capacity_left(&hashtable)) == ref_data_len),
        err_stack);

    zx_u64 fuzz_in = 0;
    ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &fuzz_in, err_stack)), err_stack);

    zx_u8 op = (zx_u8)((zx_u8)fuzz_in % 3wbu);
    if (zx_chk_sub_u32(capacity, ref_data_len) == 0) {
      ZX_ERR_ASSERT((ZXHashtable_capacity_left(&hashtable) == 0), err_stack);
      op = 0;
    }

    ZX_ERR_TRY((ZXFuzzInput_next(fuzz_input, &fuzz_in, err_stack)), err_stack);

    KeyVal kv = {0};
    static_assert(sizeof(KeyVal) == sizeof(zx_u64));
    __builtin_memcpy(&kv, &fuzz_in, sizeof(zx_u64));

    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&kv.key, sizeof(key_type), 0);

    switch (op) {
    case 0: {
      void const *entry = ZXHashtable_get(&hashtable, hash, &kv, sizeof(KeyVal),
                                          nullptr, keys_eql);

      KeyVal const *ref_entry = ref_get(ref_data, ref_data_len, kv.key);

      if (ref_entry == nullptr) {
        ZX_ERR_ASSERT((entry == nullptr), err_stack);
      } else {
        ZX_ERR_ASSERT((entry != nullptr), err_stack);

        ZX_ERR_ASSERT((kv_eql(*ref_entry, *(KeyVal const *)entry)), err_stack);
      }

      break;
    }
    case 1: {
      zx_u32 const initial_cap_left = ZXHashtable_capacity_left(&hashtable);
      void const *entry = ZXHashtable_get_or_insert(
          &hashtable, hash, &kv, sizeof(KeyVal), nullptr, keys_eql);
      zx_u32 const after_cap_left = ZXHashtable_capacity_left(&hashtable);
      bool const did_exist = initial_cap_left == after_cap_left;

      // has to either exist before ot operation did nothing.
      ZX_ERR_ASSERT((did_exist || (initial_cap_left == after_cap_left + 1)),
                    err_stack);

      // should always point to a valid entry inside hashtable.
      ZX_ERR_ASSERT((entry != nullptr), err_stack);

      KeyVal const *ref_entry = ref_get_or_insert(ref_data, &ref_data_len, kv);
      ZX_ERR_ASSERT((kv_eql(*ref_entry, *(KeyVal const *)entry)), err_stack);

      break;
    }
    case 2: {
      bool const did_exist = ZXHashtable_insert(
          &hashtable, hash, &kv, sizeof(KeyVal), nullptr, keys_eql);

      bool const ref_did_exist = ref_insert(ref_data, &ref_data_len, kv);

      ZX_ERR_ASSERT((did_exist == ref_did_exist), err_stack);

      break;
    }
    default: {
      ZX_ASSERT((false));
      break;
    }
    }
  }

  return ZXStatus_Ok;
}

int main(void) {
  constexpr zx_usize MEM_CAP = (zx_usize){1} << 30;
  constexpr zx_usize FUZZ_MEM_CAP = (zx_usize){1} << 29;

  void *mem = zx_page_alloc_thp_2mb(MEM_CAP);
  ZXAlloc alloc = ZXAlloc_init(mem, MEM_CAP);

  ZXErrStack err_stack = ZXErrStack_init();

  void *fuzz_mem = nullptr;
  ZX_ERR_TRACE_TRY_ABORT(
      ZXAlloc_alloc(&alloc, FUZZ_MEM_CAP, &fuzz_mem, &err_stack), &err_stack);

  FuzzCtx const ctx = {
      .mem = fuzz_mem,
      .cap = FUZZ_MEM_CAP,
  };

  // initiate main loop
  ZX_ERR_TRACE_TRY_ABORT(
      (zx_fuzz_main(&alloc, (void *)&ctx, fuzz_one, &err_stack)), &err_stack);

  zx_page_alloc_free(mem, MEM_CAP);
  return ZX_SYS_EXIT_SUCCESS;
}

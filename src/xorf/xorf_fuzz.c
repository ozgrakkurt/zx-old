#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xorf/xorf.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>

constexpr zx_usize MEM_ALIGN = 1 << 21;
constexpr zx_u32 MAX_N = 2048;
constexpr zx_usize MEM_SIZE =
    (sizeof(zx_u32) * MAX_N + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

static void *MEM;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  MEM = zx_page_alloc_thp_2mb(MEM_SIZE);
  return 0;
}

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  zx_u64 const *ap = (zx_u64 const *)a;
  zx_u64 const *bp = (zx_u64 const *)b;
  return *ap == *bp;
}

static ZXStatus test_one(ZXAlloc *alloc, zx_u256 const *keys, zx_u32 num_keys,
                         ZXErrStack *err_stack) {
  ZXHashtable hashtable;
  ZX_ERR_TRY((ZXHashtable_init(alloc, num_keys, alignof(zx_u64), sizeof(zx_u64),
                               &hashtable, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, num_keys * sizeof(zx_u64), (void **)&hashes,
                            err_stack)),
             err_stack);

  zx_u32 num_hashes = 0;
  for (zx_u32 i = 0; i < num_keys; i++) {
    zx_u256 const key = keys[i];
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(zx_u256), 0);

    bool const existed_before = ZXHashtable_insert(
        &hashtable, hash, &hash, sizeof(zx_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  XorfHeader header = {0};
  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                             &fingerprints, err_stack)),
             err_stack);

  for (zx_usize i = 0; i < num_hashes; i++) {
    zx_u64 const hash = hashes[i];
    ZX_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  return ZXStatus_Ok;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  constexpr zx_u32 HEAD_SIZE = sizeof(zx_u32);

  if (fuzz_input_len < HEAD_SIZE) {
    return 0;
  }

  zx_u32 const max_input_size = ((zx_u32)fuzz_input_len) - HEAD_SIZE;
  zx_u32 const max_input_len = max_input_size / sizeof(zx_u256);

  zx_u32 len = 0;
  memcpy(&len, fuzz_input, sizeof(zx_u32));

  if (max_input_len > 0) {
    len %= ZX_MIN(max_input_len, MAX_N);
  } else {
    len = 0;
  }

  zx_u32 const num_keys = len;

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(MEM, MEM_SIZE);

  zx_u256 *keys = nullptr;
  ZX_ERR_TRACE_ABORT((ZXAlloc_alloc(&alloc, sizeof(zx_u256) * num_keys,
                                    (void **)&keys, &err_stack)),
                     &err_stack);

  memcpy(keys, &fuzz_input[HEAD_SIZE], num_keys * sizeof(zx_u256));

  ZX_ERR_TRACE_ABORT((test_one(&alloc, keys, num_keys, &err_stack)),
                     &err_stack);

  return 0;
}

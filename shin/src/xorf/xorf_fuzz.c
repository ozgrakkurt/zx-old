#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>
#include <shin/shin_err_trace.h>
#include <shin/shin_hashtable.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_rapidhash_v3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xorf/xorf.h>

constexpr shin_usize MEM_ALIGN = 1 << 21;
constexpr shin_u32 MAX_N = 2048;
constexpr shin_usize MEM_SIZE =
    (sizeof(shin_u32) * MAX_N + MEM_ALIGN - 1) / MEM_ALIGN * MEM_ALIGN;

static void *MEM;

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;

  MEM = shin_page_alloc_thp_2mb(MEM_SIZE);
  return 0;
}

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  shin_u64 const *ap = (shin_u64 const *)a;
  shin_u64 const *bp = (shin_u64 const *)b;
  return *ap == *bp;
}

static ShinStatus test_one(ShinAlloc *alloc, shin_u256 const *keys,
                           shin_u32 num_keys, ShinErrStack *err_stack) {
  ShinHashtable hashtable;
  SHIN_ERR_TRY((ShinHashtable_init(alloc, num_keys, alignof(shin_u64),
                                   sizeof(shin_u64), &hashtable, err_stack)),
               err_stack);

  shin_u64 *hashes = nullptr;
  SHIN_ERR_TRY((ShinAlloc_alloc(alloc, num_keys * sizeof(shin_u64),
                                (void **)&hashes, err_stack)),
               err_stack);

  shin_u32 num_hashes = 0;
  for (shin_u32 i = 0; i < num_keys; i++) {
    shin_u256 const key = keys[i];
    shin_u64 const hash =
        shin_rapidhash_v3((shin_u8 const *)&key, sizeof(shin_u256), 0);

    bool const existed_before = ShinHashtable_insert(
        &hashtable, hash, &hash, sizeof(shin_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  XorfHeader header = {0};
  shin_u16 *fingerprints = nullptr;
  SHIN_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                               &fingerprints, err_stack)),
               err_stack);

  for (shin_usize i = 0; i < num_hashes; i++) {
    shin_u64 const hash = hashes[i];
    SHIN_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  return ShinStatus_Ok;
}

int LLVMFuzzerTestOneInput(uint8_t const *fuzz_input, size_t fuzz_input_len) {
  constexpr shin_u32 HEAD_SIZE = sizeof(shin_u32);

  if (fuzz_input_len < HEAD_SIZE) {
    return 0;
  }

  shin_u32 const max_input_size = ((shin_u32)fuzz_input_len) - HEAD_SIZE;
  shin_u32 const max_input_len = max_input_size / sizeof(shin_u256);

  shin_u32 len = 0;
  memcpy(&len, fuzz_input, sizeof(shin_u32));

  if (max_input_len > 0) {
    len %= SHIN_MIN(max_input_len, MAX_N);
  } else {
    len = 0;
  }

  shin_u32 const num_keys = len;

  ShinErrStack err_stack = ShinErrStack_init();
  ShinAlloc alloc = ShinAlloc_init(MEM, MEM_SIZE);

  shin_u256 *keys = nullptr;
  SHIN_ERR_TRACE_ABORT((ShinAlloc_alloc(&alloc, sizeof(shin_u256) * num_keys,
                                        (void **)&keys, &err_stack)),
                       &err_stack);

  memcpy(keys, &fuzz_input[HEAD_SIZE], num_keys * sizeof(shin_u256));

  SHIN_ERR_TRACE_ABORT((test_one(&alloc, keys, num_keys, &err_stack)),
                       &err_stack);

  return 0;
}

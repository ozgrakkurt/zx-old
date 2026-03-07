#include <xorf/xorf.h>
#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_usize MEMORY_SIZE = 1 << 30;
constexpr zx_u32 LEN = 1 << 20;

typedef zx_u256 key_type;

static inline bool u64_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  zx_u64 const *ap = (zx_u64 const *)a;
  zx_u64 const *bp = (zx_u64 const *)b;
  return *ap == *bp;
}

static ZXStatus benchy(ZXAlloc *alloc, ZXErrStack *err_stack) {
  constexpr zx_usize fmt_buf_cap = 1 << 12;
  char *fmt_buf_buf = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, fmt_buf_cap, (void **)&fmt_buf_buf, err_stack)),
      err_stack);

  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_buf, fmt_buf_cap);

  zx_u32 const num_keys = LEN;

  key_type *keys = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, sizeof(key_type) * num_keys, (void **)&keys,
                            err_stack)),
             err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);
  ZXXoshiro256_fill(&rand, (zx_u8 *)keys, sizeof(key_type) * num_keys);

  ZXHashtable hashtable;
  ZX_ERR_TRY((ZXHashtable_init(alloc, num_keys, alignof(zx_u64), sizeof(zx_u64),
                               &hashtable, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, num_keys * sizeof(zx_u64), (void **)&hashes,
                            err_stack)),
             err_stack);

  ZXTimer timer = ZXTimer_init();

  zx_u32 num_hashes = 0;
  for (zx_u32 i = 0; i < num_keys; i++) {
    key_type const key = keys[i];
    zx_u64 const hash =
        zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);

    bool const existed_before = ZXHashtable_insert(
        &hashtable, hash, &hash, sizeof(zx_u64), nullptr, u64_eql);

    if (!existed_before) {
      hashes[num_hashes] = hash;
      num_hashes += 1wbu;
    }
  }

  zx_u64 const dedup_time = ZXTimer_lap(&timer);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Dedup: ", err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, dedup_time, err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, " ticks\n", err_stack)),
             err_stack);

  XorfHeader header = {0};
  zx_u16 *fingerprints = nullptr;
  ZX_ERR_TRY((xorf_construct(alloc, alloc, hashes, num_hashes, &header,
                             &fingerprints, err_stack)),
             err_stack);

  zx_u64 const construct_time = ZXTimer_lap(&timer);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Construct: ", err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, construct_time, err_stack)),
             err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, " ticks\n", err_stack)),
             err_stack);

  for (zx_usize i = 0; i < num_hashes; i++) {
    zx_u64 const hash = hashes[i];
    ZX_ERR_ASSERT((xorf_check(&header, fingerprints, hash)), err_stack);
  }

  zx_u64 const query_ticks_per_elem =
      (zx_u64)ZXTimer_lap(&timer) / (zx_u64)num_hashes;
  ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Query: ", err_stack)), err_stack);
  ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, query_ticks_per_elem, err_stack)),
             err_stack);
  ZX_ERR_TRY(
      (ZXFmtBuf_format_cstr(&fmt_buf, " ticks per element.\n", err_stack)),
      err_stack);

  ZX_ERR_TRY((ZXFmtBuf_write(&fmt_buf, ZX_STDERR, err_stack)), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_thp_2mb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();

  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  for (zx_u8 i = 0; i < 5; i++) {
    ZX_ERR_TRACE_TRY_ABORT((benchy(&alloc, &err_stack)), &err_stack);
    ZXAlloc_reset(&alloc);
  }

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

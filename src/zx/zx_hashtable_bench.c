#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_err_trace.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_start.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_usize MEMORY_SIZE = 1 << 30;
constexpr zx_usize CAP = 40000;

typedef zx_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

static inline ZXStatus do_inserts(ZXAlloc *alloc, ZXErrStack *err_stack) {

  constexpr zx_usize fmt_buf_cap = 1 << 12;
  char *fmt_buf_buf = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, fmt_buf_cap, (void **)&fmt_buf_buf, err_stack)),
      err_stack);

  ZXFmtBuf fmt_buf = ZXFmtBuf_init(fmt_buf_buf, fmt_buf_cap);

  ZXHashtable table;
  ZX_ERR_TRY((ZXHashtable_init(alloc, CAP, alignof(key_type), sizeof(key_type),
                               &table, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY(
      (ZXAlloc_alloc(alloc, sizeof(zx_u64) * CAP, (void **)&hashes, err_stack)),
      err_stack);
  ZX_ASSERT((hashes != nullptr));

  for (zx_usize x = 0; x < 5; x++) {
    ZXTimer timer = ZXTimer_init();

    ZXXoshiro256 rand = ZXXoshiro256_init(0);
    for (zx_usize i = 0; i < CAP; i++) {
      key_type key = 0;
      ZXXoshiro256_fill(&rand, (zx_u8 *)&key, sizeof(key_type));

      zx_u64 const hash =
          zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);
      hashes[i] = hash;
    }

    zx_u64 const hash_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Hash: ", err_stack)),
               err_stack);
    ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, hash_ticks_per_elem, err_stack)),
               err_stack);
    ZX_ERR_TRY(
        (ZXFmtBuf_format_cstr(&fmt_buf, " ticks per element\n", err_stack)),
        err_stack);

    for (zx_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      ZXHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                         keys_eql);
    }

    zx_u64 const insert_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Insert: ", err_stack)),
               err_stack);
    ZX_ERR_TRY(
        (ZXFmtBuf_format_u64(&fmt_buf, insert_ticks_per_elem, err_stack)),
        err_stack);
    ZX_ERR_TRY(
        (ZXFmtBuf_format_cstr(&fmt_buf, " ticks per element\n", err_stack)),
        err_stack);

    for (zx_usize i = 0; i < CAP; i++) {
      key_type const key = i;
      ZX_ASSERT(
          (nullptr != ZXHashtable_get(&table, hashes[i], &key, sizeof(key_type),
                                      nullptr, keys_eql)));
    }

    zx_u64 const get_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_ERR_TRY((ZXFmtBuf_format_cstr(&fmt_buf, "Get: ", err_stack)), err_stack);
    ZX_ERR_TRY((ZXFmtBuf_format_u64(&fmt_buf, get_ticks_per_elem, err_stack)),
               err_stack);
    ZX_ERR_TRY(
        (ZXFmtBuf_format_cstr(&fmt_buf, " ticks per element\n", err_stack)),
        err_stack);
  }

  ZX_ERR_TRY((ZXFmtBuf_write(&fmt_buf, ZX_STDERR, err_stack)), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  void *const page = zx_page_alloc_thp_2mb(MEMORY_SIZE);

  ZXErrStack err_stack = ZXErrStack_init();
  ZXAlloc alloc = ZXAlloc_init(page, MEMORY_SIZE);

  ZX_ERR_TRACE_ABORT((do_inserts(&alloc, &err_stack)), &err_stack);

  zx_page_alloc_free(page, MEMORY_SIZE);
  return 0;
}

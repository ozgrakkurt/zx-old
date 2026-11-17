#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_fmt.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_io.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_rapidhash_v3.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

constexpr zx_u64 MEMORY_SIZE = (zx_u64){1} << 30;
constexpr zx_u64 CAP = 30123;

typedef zx_u256 key_type;

static inline bool keys_eql(void const *ctx, void const *a, void const *b) {
  (void)ctx;
  key_type const *ap = (key_type const *)a;
  key_type const *bp = (key_type const *)b;
  return *ap == *bp;
}

static inline ZXStatus do_inserts(ZXAlloc *alloc, ZXErrStack *err_stack) {
  constexpr zx_u32 fmt_buf_cap = 1 << 12;
  char *fmt_buf = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(char), fmt_buf_cap,
                            (zx_u8 **)&fmt_buf, err_stack)),
             err_stack);

  ZXFmt fmt = ZXFmt_init(fmt_buf, fmt_buf_cap);

  ZXHashtable table;
  ZX_ERR_TRY((ZXHashtable_init(alloc, CAP, alignof(key_type), sizeof(key_type),
                               &table, err_stack)),
             err_stack);

  zx_u64 *hashes = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(zx_u64), sizeof(zx_u64) * CAP,
                            (zx_u8 **)&hashes, err_stack)),
             err_stack);

  key_type *keys = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, alignof(key_type), sizeof(key_type) * CAP,
                            (zx_u8 **)&keys, err_stack)),
             err_stack);

  ZXXoshiro256 rand = ZXXoshiro256_init(0);

  for (zx_u32 x = 0; x < 5; x++) {
    ZXHashtable_reset(&table);

    ZXTimer timer = ZXTimer_init();

    ZXXoshiro256_fill(&rand, (zx_u8 *)keys, sizeof(key_type) * CAP);

    zx_u64 const fill_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_FMT(&fmt, "Fill: ", fill_ticks_per_elem, " ticks per element.\n");

    for (zx_u32 i = 0; i < CAP; i++) {
      key_type const key = keys[i];
      zx_u64 const hash =
          zx_rapidhash_v3((zx_u8 const *)&key, sizeof(key_type), 0);
      hashes[i] = hash;
    }

    zx_u64 const hash_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_FMT(&fmt, "Hash: ", hash_ticks_per_elem, " ticks per element.\n");

    for (zx_u32 i = 0; i < CAP; i++) {
      key_type const key = keys[i];
      ZXHashtable_insert(&table, hashes[i], &key, sizeof(key_type), nullptr,
                         keys_eql);
    }

    zx_u64 const insert_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_FMT(&fmt, "Insert: ", insert_ticks_per_elem, " ticks per element.\n");

    for (zx_u32 i = 0; i < CAP; i++) {
      key_type const key = keys[i];
      ZX_ASSERT(
          (nullptr != ZXHashtable_get(&table, hashes[i], &key, sizeof(key_type),
                                      nullptr, keys_eql)));
    }

    zx_u64 const get_ticks_per_elem = ZXTimer_lap(&timer) / CAP;
    ZX_FMT(&fmt, "Get: ", get_ticks_per_elem, " ticks per element.\n");
  }

  ZX_ERR_TRY((zx_io_print(ZX_STDERR, &fmt, err_stack)), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;

  ZX_ERR_TRY_FATAL((zx_page_alloc_thp_2mb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((do_inserts(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

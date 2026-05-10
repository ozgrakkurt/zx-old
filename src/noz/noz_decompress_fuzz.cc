#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXSliceMut<zx_u8> output;
  ZXSliceMut<zx_u8> compressed;
};

static void fuzz_one(void *usr_ctx, ZXFuzzInput *fuzz_input) {
  assert(usr_ctx != nullptr);
  assert(fuzz_input != nullptr);

  Ctx const *ctx = (Ctx const *)usr_ctx;

  zx_u64 const compressed_len =
      zx_rand_mod_u64(fuzz_input->next(), ctx->compressed.len());
  zx_u64 const compressed_offset =
      zx_sub_unsafe_u64(ctx->compressed.len(), compressed_len);

  zx_u64 const output_len =
      zx_rand_mod_u64(fuzz_input->next(), ctx->output.len());
  zx_u64 const output_offset = zx_sub_unsafe_u64(ctx->output.len(), output_len);

  ZXSliceMut<zx_u8> const output =
      ctx->output.slice_unsafe(output_offset, output_len);
  ZXSliceMut<zx_u8> const compressed =
      ctx->compressed.slice_unsafe(compressed_offset, compressed_len);

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());

  zx_u64 offset = 0;
  for (zx_u64 n_iter = 0; offset < compressed.len() && n_iter < 100; n_iter++) {
    zx_u64 const diff = zx_sub_unsafe_u64(compressed.len(), offset);
    zx_u64 const len = zx_rand_mod_u64(fuzz_input->next(), diff);
    ZXSliceMut<zx_u8> const slice = compressed.slice_unsafe(offset, len);

    zx_u64 const data_kind = fuzz_input->next() % 3;
    switch (data_kind) {
    case 0: {
      rand.fill(slice);
      break;
    }
    case 1: {
      slice.memset(0);
      break;
    }
    case 2: {
      slice.memset(0xaa);
      break;
    }
    default: {
      __builtin_trap();
      break;
    }
    }

    offset = zx_add_unsafe_u64(offset, len);
  }
  rand.fill(compressed.slice_at_unsafe(offset));

  zx_u64 n_read = ZX_U64_MAX;
  (void)noz_decompress(compressed.as_const(), output, &n_read);
}

static ZXSliceMut<zx_u8> make_alloc(zx_u64 capacity) {
  assert(zx_is_pow_two_u64(capacity));
  assert(capacity >= 4096);

  ZXSliceMut<zx_u8> alloc;
  ZXPageAllocStatus alloc_status =
      zx_page_alloc_4kb(zx_mul_unsafe_u64(capacity, 2), &alloc);
  assert(alloc_status == ZXPageAllocStatus::Ok);
  ZXSysStatus sys_status =
      zx_sys_mprotect(alloc.slice_at_unsafe(capacity), ZX_SYS_PROT_NONE);
  assert(sys_status == ZXSysStatus::Ok);

  ZXSliceMut<zx_u8> ret = alloc.slice_unsafe(0, capacity);
  ret.memset(0);

  return ret;
}

int main(void) {
  constexpr zx_u64 fuzz_mem_cap = zx_u64{1} << 27;
  ZXSliceMut<zx_u8> fuzz_mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_4kb(fuzz_mem_cap, &fuzz_mem));
  ZXAlloc alloc = ZXAlloc::init(fuzz_mem);
  Ctx const ctx = Ctx{
      .output = make_alloc(4096),
      .compressed = make_alloc(8192),
  };
  zx_fuzz(&alloc, (void *)&ctx, fuzz_one);
  return 0;
}

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
  ZXSliceMut<zx_u8> input;
  ZXSliceMut<zx_u8> output;
  ZXSliceMut<zx_u8> compressed;
  ZXSliceMut<zx_u8> compressed_copy;
};

static void fuzz_one(void* usr_ctx, ZXFuzzInput* fuzz_input) {
  assert(usr_ctx != nullptr);
  assert(fuzz_input != nullptr);

  Ctx const* ctx = (Ctx const*)usr_ctx;

  zx_u64 const capacity = ctx->input.len();
  assert(capacity == ctx->output.len());
  zx_u64 const len = zx_mod_unsafe_u64(fuzz_input->next(), capacity);

  zx_u64 const offset = zx_sub_unsafe_u64(capacity, len);
  ZXSliceMut<zx_u8> const input = ctx->input.slice_unsafe(offset, len);
  ZXSliceMut<zx_u8> const output = ctx->output.slice_unsafe(offset, len);
  ZXSliceMut<zx_u8> const compressed = ctx->compressed;
  ZXSliceMut<zx_u8> const compressed_copy = ctx->compressed_copy;

  ZXXoshiro256 rand = ZXXoshiro256::init(fuzz_input->next());

  zx_u64 const data_kind = fuzz_input->next() % 3;
  switch (data_kind) {
    case 0: {
      rand.fill(input);
      break;
    }
    case 1: {
      input.memset(0);
      break;
    }
    case 2: {
      input.memset(0xaa);
      break;
    }
    default: {
      __builtin_trap();
      break;
    }
  }

  zx_u64 bound = ZX_U64_MAX;
  assert(NozCompressBoundStatus::Ok == noz_compress_bound(input.len(), &bound));
  assert(bound <= compressed.len());

  compressed.fill(0xaa);
  zx_u64 n_written = ZX_U64_MAX;
  assert(NozCompressStatus::Ok ==
         noz_compress(input.as_const(), compressed, &n_written));
  assert(n_written <= bound);

  ZXSliceMut<zx_u8> copy = compressed_copy.slice_unsafe(
      zx_sub_unsafe_u64(compressed_copy.len(), n_written), n_written);
  copy.memcpy_from_unsafe(compressed.as_const().slice_unsafe(0, n_written));

  zx_u64 n_read = ZX_U64_MAX;
  assert(NozDecompressStatus::Ok ==
         noz_decompress(copy.as_const(), output, &n_read));
  assert(n_read == n_written);
  assert(n_read == copy.len());
  assert(input.as_const().equals(output.as_const()));
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
      .input = make_alloc(4096),
      .output = make_alloc(4096),
      .compressed = make_alloc(8192),
      .compressed_copy = make_alloc(8192),
  };
  zx_fuzz(&alloc, (void*)&ctx, fuzz_one);
  return 0;
}

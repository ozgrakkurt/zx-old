#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void do_test(ZXSliceMut<zx_u8> input,
                    ZXSliceMut<zx_u8> output,
                    ZXSliceMut<zx_u8> compressed,
                    ZXSliceMut<zx_u8> compressed_copy) {
  assert(input.len() == output.len());

  zx_u64 bound = ZX_U64_MAX;
  assert(NozCompressBoundStatus::Ok == noz_compress_bound(input.len(), &bound));

  assert(bound == compressed.len());

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

  return alloc.slice_unsafe(0, capacity);
}

int main(void) {
  constexpr zx_u64 capacity = 4096;

  ZXSliceMut<zx_u8> const buf0 = make_alloc(capacity);
  ZXSliceMut<zx_u8> const buf1 = make_alloc(capacity);
  ZXSliceMut<zx_u8> const buf2 = make_alloc(capacity);
  ZXSliceMut<zx_u8> const buf3 = make_alloc(capacity);

  constexpr zx_u64 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129};
  constexpr zx_u64 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  ZXXoshiro256 rand = ZXXoshiro256::init(69);

  for (zx_u64 i = 0; i < n_lengths; i++) {
    zx_u64 const len = lengths[i];

    zx_u64 bound = ZX_U64_MAX;
    assert(NozCompressBoundStatus::Ok == noz_compress_bound(len, &bound));

    zx_u64 const offset = zx_sub_unsafe_u64(capacity, len);
    ZXSliceMut<zx_u8> b0 = buf0.slice_unsafe(offset, len);
    ZXSliceMut<zx_u8> b1 = buf1.slice_unsafe(offset, len);

    zx_u64 const compr_offset = zx_sub_unsafe_u64(capacity, bound);
    ZXSliceMut<zx_u8> b2 = buf2.slice_unsafe(compr_offset, bound);
    ZXSliceMut<zx_u8> b3 = buf3.slice_unsafe(compr_offset, bound);

    b0.memset(0);
    do_test(b0, b1, b2, b3);
    b0.memset(1);
    do_test(b0, b1, b2, b3);
    rand.fill(b0);
    do_test(b0, b1, b2, b3);
  }

  return 0;
}

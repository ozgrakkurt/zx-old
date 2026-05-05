#include <noz/noz.h>
#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void do_memcpy(ZXFmt* fmt,
                      ZXTimeInfo time_info,
                      ZXSlice<zx_u8> input,
                      ZXSliceMut<zx_u8> compressed,
                      ZXSliceMut<zx_u8> output) {
  assert(input.len() == output.len());
  assert(compressed.len() >= input.len());
  ZXSliceMut<zx_u8> compr = compressed.slice_unsafe(0, input.len());

  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  compr.memcpy_from_unsafe(input);

  zx_u64 lap = timer.lap();

  bool overflow = false;
  fmt->push_cstring(ZXCString::init_unsafe("MemCpyIn: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  zx_u64 mbs = input.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  timer.lap();

  output.memcpy_from_unsafe(compr.as_const());

  lap = timer.lap();

  fmt->push_cstring(ZXCString::init_unsafe("MemCpyOut: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  mbs = input.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  assert(!overflow);

  assert(input.equals(output.as_const()));
}

static void do_noz(ZXFmt* fmt,
                   ZXTimeInfo time_info,
                   ZXSlice<zx_u8> input,
                   ZXSliceMut<zx_u8> compressed,
                   ZXSliceMut<zx_u8> output) {
  assert(input.len() == output.len());
  assert(compressed.len() >= input.len());

  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  zx_u64 n_written = 0;
  assert(NozCompressStatus::Ok == noz_compress(input, compressed, &n_written));

  zx_u64 lap = timer.lap();

  bool overflow = false;
  fmt->push_cstring(ZXCString::init_unsafe("NozCompress: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  zx_u64 mbs = input.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  timer.lap();

  zx_u64 n_read = 0;
  assert(NozDecompressStatus::Ok ==
         noz_decompress(compressed.as_const(), output, &n_read));
  assert(n_read == n_written);

  lap = timer.lap();

  fmt->push_cstring(ZXCString::init_unsafe("NozDecompress: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  mbs = input.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  assert(!overflow);
  assert(input.equals(output.as_const()));
}

static void roundtrip(ZXAlloc* alloc, zx_u64 len) {
  ZXTimeInfo time_info;
  assert(ZXTimeStatus::Ok == ZXTimeInfo::init(&time_info));

  // Mess with alignment
  ZXSliceMut<zx_u8> nouse;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<zx_u8> input;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(len, &input));

  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  // Mess with alignment
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  zx_u64 compress_bound = ZX_U64_MAX;
  assert(NozCompressBoundStatus::Ok ==
         noz_compress_bound(len, &compress_bound));

  ZXSliceMut<zx_u8> compressed;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(compress_bound, &compressed));
  compressed.fill(0xaa);

  // Mess with alignment
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<zx_u8> output;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(len, &output));
  output.fill(0xaa);

  constexpr zx_u32 FMT_CAP = 1 << 16;
  ZXSliceMut<zx_u8> fmt_buf;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(FMT_CAP, &fmt_buf));
  ZXFmt fmt = ZXFmt::init(fmt_buf);

  rand.fill(input);
  rand.fill(compressed);
  rand.fill(output);

  do_noz(&fmt, time_info, input.as_const(), compressed, output);

  rand.fill(input);
  rand.fill(compressed);
  rand.fill(output);

  do_memcpy(&fmt, time_info, input.as_const(), compressed, output);

  zx_u64 n_written = 0;
  assert(ZXSysStatus::Ok == zx_sys_write(ZX_STDERR, fmt.written(), &n_written));
  assert(n_written == fmt.n_written());
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = (zx_u64){1} << 32;
  constexpr zx_u64 LEN = ((zx_u64){1} << 30) + 5;

  ZXSliceMut<zx_u8> mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_thp_2mb(MEMORY_SIZE, &mem));

  ZXAlloc alloc = ZXAlloc::init(mem);

  roundtrip(&alloc, LEN);

  assert(ZXPageAllocStatus::Ok == zx_page_free(mem));

  return 0;
}

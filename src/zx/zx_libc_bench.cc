#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>
#include "./zx_libc.h"

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void do_strlen(ZXFmt* fmt, ZXTimeInfo time_info, char const* str) {
  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  zx_u64 const len = zx_strlen(str);

  zx_u64 lap = timer.lap();

  bool overflow = false;
  fmt->push_cstring(ZXCString::init_unsafe("strlen: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  zx_u64 mbs = len * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  assert(!overflow);
}

static void do_memcpy(ZXFmt* fmt,
                      ZXTimeInfo time_info,
                      ZXSlice<zx_u8> src,
                      ZXSliceMut<zx_u8> dst) {
  assert(src.len() == dst.len());

  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  zx_memcpy(dst.ptr(), src.ptr(), src.len());

  zx_u64 lap = timer.lap();

  bool overflow = false;
  fmt->push_cstring(ZXCString::init_unsafe("memcpy: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  zx_u64 mbs = src.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  timer.lap();

  assert(0 == zx_memcmp(dst.ptr(), src.ptr(), src.len()));

  lap = timer.lap();

  fmt->push_cstring(ZXCString::init_unsafe("memcmp: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  mbs = src.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  assert(!overflow);
}

static void do_memset(ZXFmt* fmt,
                      ZXTimeInfo time_info,
                      zx_u8 src,
                      ZXSliceMut<zx_u8> dst) {
  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  zx_memset(dst.ptr(), src, dst.len());

  zx_u64 const lap = timer.lap();

  bool overflow = false;
  fmt->push_cstring(ZXCString::init_unsafe("memset: "), &overflow);
  fmt->push_u64(lap, &overflow);
  fmt->push_cstring(ZXCString::init_unsafe(" ns. "), &overflow);

  zx_u64 mbs = dst.len() * 1'000'000'000__wbu / (lap * 1024__wbu * 1024__wbu);
  fmt->push_cstring(ZXCString::init_unsafe("MB/s: "), &overflow);
  fmt->push_u64(mbs, &overflow);
  fmt->push_char('\n', &overflow);

  assert(!overflow);

  for (zx_u64 i = 0; i < dst.len(); i++) {
    assert(dst.read_unsafe(i) == src);
  }
}

static void roundtrip(ZXAlloc* alloc, zx_u64 len) {
  ZXTimeInfo time_info;
  assert(ZXTimeStatus::Ok == ZXTimeInfo::init(&time_info));

  // Mess with alignment
  ZXSliceMut<zx_u8> nouse;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<zx_u8> src;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(len, &src));

  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  // Mess with alignment
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<zx_u8> dst;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(len, &dst));
  dst.fill(0xaa);

  constexpr zx_u32 FMT_CAP = 1 << 16;
  ZXSliceMut<zx_u8> fmt_buf;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(FMT_CAP, &fmt_buf));
  ZXFmt fmt = ZXFmt::init(fmt_buf);

  rand.fill(src);
  rand.fill(dst);
  do_memcpy(&fmt, time_info, src.as_const(), dst);

  rand.fill(dst);
  do_memset(&fmt, time_info, zx_u8{111}, dst);

  src.memset(0xaa);
  src.write_unsafe(zx_sub_unsafe_u64(src.len(), 1), zx_u8{69});
  do_strlen(&fmt, time_info, (char const*)src.ptr());

  zx_u64 n_written = 0;
  assert(ZXSysStatus::Ok == zx_sys_write(ZX_STDERR, fmt.written(), &n_written));
  assert(n_written == fmt.n_written());
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = (zx_u64){1} << 32;
  constexpr zx_u64 LEN = zx_u64{1} << 30;

  ZXSliceMut<zx_u8> mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_thp_2mb(MEMORY_SIZE, &mem));

  ZXAlloc alloc = ZXAlloc::init(mem);

  roundtrip(&alloc, LEN);

  assert(ZXPageAllocStatus::Ok == zx_page_free(mem));

  return 0;
}

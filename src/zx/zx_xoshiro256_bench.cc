#include <zx/zx_alloc.h>
#include <zx/zx_fmt.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>
#include <zx/zx_time.h>
#include <zx/zx_xoshiro256.h>

static void assert(bool cond) {
  if (!cond) {
    zx_panic();
  }
}

template <typename T>
static zx_u64 do_loop(ZXAlloc *alloc, ZXTimeInfo time_info, zx_u64 len) {
  // Mess with alignment
  ZXSliceMut<zx_u8> nouse;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<T> input;
  assert(ZXAllocStatus::Ok == alloc->alloc<T>(len, &input));

  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  for (zx_u64 i = 0; i < len; i++) {
    input.write_unsafe(i, rand.random<T>());
  }

  zx_u64 const lap = timer.lap();

  return lap;
}

template <typename T>
static zx_u64 do_fill(ZXAlloc *alloc, ZXTimeInfo time_info, zx_u64 len) {
  // Mess with alignment
  ZXSliceMut<zx_u8> nouse;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(69, &nouse));

  ZXSliceMut<T> input;
  assert(ZXAllocStatus::Ok == alloc->alloc<T>(len, &input));

  ZXXoshiro256 rand = ZXXoshiro256::init(0);

  ZXTimer timer = ZXTimer::init(time_info);

  timer.lap();

  rand.fill(input.template scast<zx_u8>());

  zx_u64 const lap = timer.lap();

  return lap;
}

static void roundtrip(ZXAlloc *alloc, ZXTimeInfo time_info, zx_u64 len) {
  alloc->reset();
  zx_u64 const fill_bytes = do_fill<zx_u8>(alloc, time_info, len);
  alloc->reset();
  zx_u64 const fill_u64 =
      do_fill<zx_u64>(alloc, time_info, zx_div_unsafe_u64(len, sizeof(zx_u64)));
  alloc->reset();
  zx_u64 const fill_u256 = do_fill<zx_u256>(
      alloc, time_info, zx_div_unsafe_u64(len, sizeof(zx_u256)));
  alloc->reset();
  zx_u64 const loop_bytes = do_loop<zx_u8>(alloc, time_info, len);
  alloc->reset();
  zx_u64 const loop_u64 =
      do_loop<zx_u64>(alloc, time_info, zx_div_unsafe_u64(len, sizeof(zx_u64)));
  alloc->reset();
  zx_u64 const loop_u256 = do_loop<zx_u256>(
      alloc, time_info, zx_div_unsafe_u64(len, sizeof(zx_u256)));
  alloc->reset();

  constexpr zx_u64 FMT_CAP = 1 << 14;
  ZXSliceMut<zx_u8> fmt_buf;
  assert(ZXAllocStatus::Ok == alloc->alloc<zx_u8>(FMT_CAP, &fmt_buf));
  ZXFmt fmt = ZXFmt::init(fmt_buf);

  bool overflow = false;

  fmt.push_cstring(ZXCString::init_unsafe("FillBytes: \t"), &overflow);
  fmt.push_u64(fill_bytes, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  fmt.push_cstring(ZXCString::init_unsafe("Fill_U64: \t"), &overflow);
  fmt.push_u64(fill_u64, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  fmt.push_cstring(ZXCString::init_unsafe("Fill_U256: \t"), &overflow);
  fmt.push_u64(fill_u256, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  fmt.push_cstring(ZXCString::init_unsafe("LoopBytes: \t"), &overflow);
  fmt.push_u64(loop_bytes, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  fmt.push_cstring(ZXCString::init_unsafe("Loop_U64: \t"), &overflow);
  fmt.push_u64(loop_u64, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  fmt.push_cstring(ZXCString::init_unsafe("Loop_U256: \t"), &overflow);
  fmt.push_u64(loop_u256, &overflow);
  fmt.push_cstring(ZXCString::init_unsafe(" ns.\n"), &overflow);

  assert(!overflow);

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

  ZXTimeInfo time_info;
  assert(ZXTimeStatus::Ok == ZXTimeInfo::init(&time_info));

  roundtrip(&alloc, time_info, LEN);

  assert(ZXPageAllocStatus::Ok == zx_page_free(mem));

  return 0;
}

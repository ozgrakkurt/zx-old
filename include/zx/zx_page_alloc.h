#ifndef ZX_PAGE_ALLOC_H
#define ZX_PAGE_ALLOC_H

#ifndef __linux__
#error "expected linux"
#endif

#ifndef __x86_64__
#error "expected x86_64"
#endif

#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

enum class [[nodiscard]] ZXPageAllocStatus : zx_u8_c {
  Ok = 0,
  Nullptr,
  IntegerOverflow,
  InvalidSize,
  OsRetNullptr,
  SysErr,
  SliceInitErr,
  MadviseFail,
};

static inline ZXCString
zx_page_alloc_status_to_cstring(ZXPageAllocStatus status) {
  switch (status) {
  case ZXPageAllocStatus::Ok: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::Ok");
  }
  case ZXPageAllocStatus::Nullptr: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::Nullptr");
  }
  case ZXPageAllocStatus::IntegerOverflow: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::IntegerOverflow");
  }
  case ZXPageAllocStatus::InvalidSize: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::InvalidSize");
  }
  case ZXPageAllocStatus::OsRetNullptr: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::OsRetNullptr");
  }
  case ZXPageAllocStatus::SysErr: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::SysErr");
  }
  case ZXPageAllocStatus::SliceInitErr: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::SliceInitErr");
  }
  case ZXPageAllocStatus::MadviseFail: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::MadviseFail");
  }
  default: {
    return ZXCString::init_unsafe("ZXPageAllocStatus::Unknown");
  }
  }
}

static inline ZXPageAllocStatus
zx_page_mmap_(zx_u64 size, zx_i32 huge_page_flags, ZXSliceMut<zx_u8> *out) {
  if (out == nullptr) {
    return ZXPageAllocStatus::Nullptr;
  }

  if (!zx_is_pow_two_u64(size) || size < (zx_u64{1} << 12)) {
    return ZXPageAllocStatus::InvalidSize;
  }

  zx_i32 const prot = ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE;
  zx_i32 const flags = ZX_SYS_MAP_PRIVATE | ZX_SYS_MAP_ANONYMOUS |
                       ZX_SYS_MAP_POPULATE | huge_page_flags;

  zx_u8 *mem = nullptr;
  ZXSysStatus const sys_status =
      zx_sys_mmap(nullptr, size, prot, flags, -1, 0, &mem);
  if (sys_status != ZXSysStatus::Ok) {
    return ZXPageAllocStatus::SysErr;
  }
  if (mem == nullptr) {
    return ZXPageAllocStatus::OsRetNullptr;
  }

  ZXSliceStatus const slice_status = ZXSliceMut<zx_u8>::init(mem, size, out);
  if (slice_status != ZXSliceStatus::Ok) {
    return ZXPageAllocStatus::SliceInitErr;
  }

  return ZXPageAllocStatus::Ok;
}

static inline ZXPageAllocStatus zx_page_alloc_4kb(zx_u64 size,
                                                  ZXSliceMut<zx_u8> *out) {
  if (out == nullptr) {
    return ZXPageAllocStatus::Nullptr;
  }

  if (!zx_is_pow_two_u64(size) || size < (zx_u64{1} << 12)) {
    return ZXPageAllocStatus::InvalidSize;
  }
  return zx_page_mmap_(size, 0, out);
}

static inline ZXPageAllocStatus zx_page_alloc_thp_2mb(zx_u64 size,
                                                      ZXSliceMut<zx_u8> *out) {
  if (out == nullptr) {
    return ZXPageAllocStatus::Nullptr;
  }

  if (!zx_is_pow_two_u64(size) || size < (zx_u64{1} << 21)) {
    return ZXPageAllocStatus::InvalidSize;
  }

  ZXSliceMut<zx_u8> mem;
  ZXPageAllocStatus const mmap_status = zx_page_mmap_(size, 0, &mem);
  if (mmap_status != ZXPageAllocStatus::Ok) {
    return mmap_status;
  }

  ZXSysStatus const madvise_status = zx_sys_madvise(mem, ZX_SYS_MADV_HUGEPAGE);
  if (madvise_status != ZXSysStatus::Ok) {
    return ZXPageAllocStatus::MadviseFail;
  }

  *out = mem;
  return ZXPageAllocStatus::Ok;
}

static inline ZXPageAllocStatus zx_page_alloc_huge_2mb(zx_u64 size,
                                                       ZXSliceMut<zx_u8> *out) {
  if (out == nullptr) {
    return ZXPageAllocStatus::Nullptr;
  }

  if (!zx_is_pow_two_u64(size) || size < (zx_u64{1} << 21)) {
    return ZXPageAllocStatus::InvalidSize;
  }

  return zx_page_mmap_(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_2MB, out);
}

static inline ZXPageAllocStatus zx_page_alloc_huge_1gb(zx_u64 size,
                                                       ZXSliceMut<zx_u8> *out) {
  if (out == nullptr) {
    return ZXPageAllocStatus::Nullptr;
  }

  if (!zx_is_pow_two_u64(size) || size < (zx_u64{1} << 30)) {
    return ZXPageAllocStatus::InvalidSize;
  }

  return zx_page_mmap_(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_1GB, out);
}

static inline ZXPageAllocStatus zx_page_free(ZXSliceMut<zx_u8> mem) {
  ZXSysStatus const status = zx_sys_munmap(mem);
  if (status != ZXSysStatus::Ok) {
    return ZXPageAllocStatus::SysErr;
  }
  return ZXPageAllocStatus::Ok;
}

#endif

#ifndef ZX_TIME_H
#define ZX_TIME_H

#ifndef __x86_64__
#error "only x86_64 targets are supported"
#endif

#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_perf_event.h>
#include <zx/zx_slice.h>
#include <zx/zx_sys.h>

enum class [[nodiscard]] ZXTimeStatus : zx_u8_c {
  Ok = 0,
  PerfEventOpenErr,
  MmapErr,
  NotSupported,
  InvalidNum,
  MunmapErr,
  CloseErr,
};

static inline ZXCString zx_time_status_to_cstring(ZXTimeStatus status) {
  switch (status) {
  case ZXTimeStatus::Ok: {
    return ZXCString::init_unsafe("ZXTimeStatus::Ok");
  }
  case ZXTimeStatus::PerfEventOpenErr: {
    return ZXCString::init_unsafe("ZXTimeStatus::PerfEventOpenErr");
  }
  case ZXTimeStatus::MmapErr: {
    return ZXCString::init_unsafe("ZXTimeStatus::MmapErr");
  }
  case ZXTimeStatus::NotSupported: {
    return ZXCString::init_unsafe("ZXTimeStatus::NotSupported");
  }
  case ZXTimeStatus::InvalidNum: {
    return ZXCString::init_unsafe("ZXTimeStatus::InvalidNum");
  }
  case ZXTimeStatus::MunmapErr: {
    return ZXCString::init_unsafe("ZXTimeStatus::MunmapErr");
  }
  case ZXTimeStatus::CloseErr: {
    return ZXCString::init_unsafe("ZXTimeStatus::CloseErr");
  }
  default: {
    return ZXCString::init_unsafe("ZXTimeStatus::Unknown");
  }
  }
}

struct ZXTimeInfo {
  zx_u64 m_time_shift;
  zx_u64 m_time_mult;

  static inline ZXTimeStatus init(ZXTimeInfo *out) {
    perf_event_attr pattr;
    __builtin_memset(&pattr, 0, sizeof(pattr));
    pattr.type = PERF_TYPE_HARDWARE;
    pattr.size = sizeof(pattr);
    pattr.config = PERF_COUNT_HW_INSTRUCTIONS;
    pattr.disabled = 1;
    pattr.exclude_kernel = 1;
    pattr.exclude_hv = 1;

    ZXTimeStatus status = ZXTimeStatus::Ok;
    zx_i32 fd = -1;
    perf_event_mmap_page *mmap_page = nullptr;

    ZXSysStatus sys_status = zx_sys_perf_event_open(&pattr, 0, -1, -1, 0, &fd);
    if (sys_status != ZXSysStatus::Ok) {
      status = ZXTimeStatus::PerfEventOpenErr;
      goto cleanup;
    }
    zx_assert_dev(fd != -1);

    sys_status = zx_sys_mmap(nullptr, 4096, ZX_SYS_PROT_READ, ZX_SYS_MAP_SHARED,
                             fd, 0, (zx_u8 **)&mmap_page);
    if (sys_status != ZXSysStatus::Ok) {
      status = ZXTimeStatus::MmapErr;
      goto cleanup;
    }
    zx_assert_dev(mmap_page != nullptr);

    if (mmap_page->cap_user_time != 1) {
      status = ZXTimeStatus::NotSupported;
      goto cleanup;
    }

    {
      ZXTimeInfo const info = ZXTimeInfo{
          .m_time_shift = mmap_page->time_shift,
          .m_time_mult = mmap_page->time_mult,
      };

      // This would cause ub when we are converting cycles to nanos later.
      if (info.m_time_shift >= 128 || info.m_time_mult == 0) {
        status = ZXTimeStatus::InvalidNum;
        goto cleanup;
      }

      *out = info;
    }
  cleanup:
    if (mmap_page != nullptr) {
      ZXSliceMut<zx_u8> slice =
          ZXSliceMut<zx_u8>::init_unsafe((zx_u8 *)mmap_page, 4096);
      sys_status = zx_sys_munmap(slice);
      if (sys_status != ZXSysStatus::Ok) {
        status = ZXTimeStatus::MunmapErr;
      }
    }
    if (fd != -1) {
      sys_status = zx_sys_close(fd);
      if (sys_status != ZXSysStatus::Ok) {
        status = ZXTimeStatus::CloseErr;
      }
    }

    return status;
  }

  inline zx_u64 cycles_to_nanos(zx_u64 cycles) const {
    zx_u128 const x = cycles;
    zx_assert_dev(this->m_time_shift < 128);
    return (zx_u64)((x * this->m_time_mult) >> this->m_time_shift);
  }
};

struct ZXTimer {
  ZXTimeInfo m_info;
  zx_u64 m_start;

  static inline zx_u64 now_() {
    unsigned int aux;
    zx_u64 const ticker = __builtin_ia32_rdtscp(&aux);
    (void)aux;

    return ticker;
  }

  static inline ZXTimer init(ZXTimeInfo info) {
    zx_u64 const start = now_();

    return ZXTimer{
        .m_start = start,
        .m_info = info,
    };
  }

  // Returns the number of nanoseconds elapsed since last lap.
  // and resets the lap timer.
  inline zx_u64 lap() {
    zx_u64 const now = now_();
    zx_u64 const diff = zx_sub_unsafe_u64(now, this->m_start);
    this->m_start = now;
    return this->m_info.cycles_to_nanos(diff);
  }

  // Returns the number of nanoseconds elapsed since last lap.
  inline zx_u64 elapsed() const {
    zx_u64 const now = now_();
    zx_u64 const diff = zx_sub_unsafe_u64(now, this->m_start);
    return this->m_info.cycles_to_nanos(diff);
  }
};

#endif

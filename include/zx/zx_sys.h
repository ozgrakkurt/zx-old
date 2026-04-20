#ifndef ZX_SYS_H
#define ZX_SYS_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_int.h>
#include <zx/zx_slice.h>
#include <zx/zx_syscall.h>

enum class [[nodiscard]] ZXSysStatus : unsigned char {
  Ok = 0,
  UnexpectedError,
  Nullptr,
};

constexpr zx_u64 ZX_SYS_STATUS_SUCCESS = 0;

static inline zx_u64 zx_sys_errno(zx_u64 ret) {
  zx_i64 const sgnd = (zx_i64)ret;
  if (sgnd > -4096 && sgnd < 0) {
    return (zx_u64)-sgnd;
  } else {
    return ZX_SYS_STATUS_SUCCESS;
  }
}

static inline ZXSysStatus zx_sys_status(zx_u64 ret) {
  switch (zx_sys_errno(ret)) {
    case ZX_SYS_STATUS_SUCCESS: {
      return ZXSysStatus::Ok;
    }
    default: {
      return ZXSysStatus::UnexpectedError;
    }
  }
}

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "read"
constexpr zx_u64 ZX_SYSCALL_READ = 0;
constexpr zx_u64 ZX_SYSCALL_WRITE = 1;
constexpr zx_u64 ZX_SYSCALL_OPEN = 2;
constexpr zx_u64 ZX_SYSCALL_CLOSE = 3;
constexpr zx_u64 ZX_SYSCALL_MMAP = 9;
constexpr zx_u64 ZX_SYSCALL_MPROTECT = 10;
constexpr zx_u64 ZX_SYSCALL_MUNMAP = 11;
constexpr zx_u64 ZX_SYSCALL_MADVISE = 28;
constexpr zx_u64 ZX_SYSCALL_EXIT_GROUP = 231;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "io_uring"
constexpr zx_u64 ZX_SYSCALL_IO_URING_SETUP = 425;
constexpr zx_u64 ZX_SYSCALL_IO_URING_ENTER = 426;
constexpr zx_u64 ZX_SYSCALL_IO_URING_REGISTER = 427;

static inline ZXSysStatus zx_sys_read(zx_i32 fd,
                                      ZXSliceMut<zx_u8> buf,
                                      zx_u64* n_read) {
  if (n_read == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const read_size = zx_min_u64(0x7ffff000, buf.len());

  zx_u64 const ret =
      zx_syscall3(ZX_SYSCALL_READ, (zx_u64)fd, (zx_u64)buf.ptr(), read_size);

  ZXSysStatus const status = zx_sys_status(ret);
  if (status != ZXSysStatus::Ok) {
    *n_read = 0;
    return status;
  }
  *n_read = ret;
  return ZXSysStatus::Ok;
}

static inline ZXSysStatus zx_sys_write(zx_i32 fd,
                                       ZXSliceMut<zx_u8> buf,
                                       zx_u64* n_written) {
  if (n_written == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const write_size = zx_min_u64(0x7ffff000, buf.len());

  zx_u64 const ret =
      zx_syscall3(ZX_SYSCALL_WRITE, (zx_u64)fd, (zx_u64)buf.ptr(), write_size);

  ZXSysStatus const status = zx_sys_status(ret);
  if (status != ZXSysStatus::Ok) {
    *n_written = 0;
    return status;
  }
  *n_written = ret;
  return ZXSysStatus::Ok;
}

static inline ZXSysStatus zx_sys_open(ZXCString pathname,
                                      zx_i32 flags,
                                      zx_u32 mode,
                                      zx_i32* fd) {
  if (fd == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const ret = zx_syscall3(ZX_SYSCALL_OPEN, (zx_u64)pathname.as_cstr(),
                                 (zx_u64)flags, mode);

  ZXSysStatus const status = zx_sys_status(ret);
  if (status != ZXSysStatus::Ok) {
    *fd = -1;
    return status;
  }
  zx_i32 const ret_fd = (zx_i32)ret;
  if (ret_fd <= 0) {
    *fd = -1;
    return ZXSysStatus::UnexpectedError;
  }
  *fd = ret_fd;
  return ZXSysStatus::Ok;
}

static inline ZXSysStatus zx_sys_close(zx_i32 fd) {
  zx_u64 const ret = zx_syscall1(ZX_SYSCALL_CLOSE, (zx_u64)fd);
  return zx_sys_status(ret);
}

[[noreturn]]
static inline void zx_sys_abort() {
  zx_syscall1(ZX_SYSCALL_EXIT_GROUP, 127);
  __builtin_trap();
}

// returns the resulting pointer through `mem` argument
static inline ZXSysStatus zx_sys_mmap(zx_u8* address,
                                      zx_u64 length,
                                      zx_i32 prot,
                                      zx_i32 flags,
                                      zx_i32 fd,
                                      zx_i64 offset,
                                      zx_u8** mem) {
  if (mem == nullptr) {
    return ZXSysStatus::Nullptr;
  }

  zx_u64 const ret =
      zx_syscall6(ZX_SYSCALL_MMAP, (zx_u64)address, length, (zx_u64)prot,
                  (zx_u64)flags, (zx_u64)fd, (zx_u64)offset);

  ZXSysStatus const status = zx_sys_status(ret);
  if (status != ZXSysStatus::Ok) {
    *mem = nullptr;
    return status;
  }

  *mem = (zx_u8*)ret;
  return ZXSysStatus::Ok;
}

static inline ZXSysStatus zx_sys_munmap(zx_u8* mem, zx_u64 length) {
  zx_u64 const ret = zx_syscall2(ZX_SYSCALL_MUNMAP, (zx_u64)mem, length);
  return zx_sys_status(ret);
}

static inline ZXSysStatus zx_sys_madvise(zx_u8* mem,
                                         zx_u64 length,
                                         zx_i32 advice) {
  zx_u64 const ret =
      zx_syscall3(ZX_SYSCALL_MADVISE, (zx_u64)mem, length, (zx_u64)advice);
  return zx_sys_status(ret);
}

static inline ZXSysStatus zx_sys_mprotect(zx_u8* mem,
                                          zx_u64 length,
                                          zx_i32 prot) {
  zx_u64 const ret =
      zx_syscall3(ZX_SYSCALL_MPROTECT, (zx_u64)mem, length, (zx_u64)prot);
  return zx_sys_status(ret);
}

// echo '#include <sys/mman.h>' | gcc -E - -dM | grep "#define MAP_"
constexpr zx_i32 ZX_SYS_MAP_SHARED = 0x01;
constexpr zx_i32 ZX_SYS_MAP_PRIVATE = 0x02;
constexpr zx_i32 ZX_SYS_MAP_ANONYMOUS = 0x20;
constexpr zx_i32 ZX_SYS_MAP_LOCKED = 0x02000;
constexpr zx_i32 ZX_SYS_MAP_POPULATE = 0x08000;
constexpr zx_i32 ZX_SYS_MAP_HUGETLB = 0x40000;

constexpr zx_i32 ZX_SYS_MAP_HUGE_SHIFT = 26;
constexpr zx_i32 ZX_SYS_MAP_HUGE_2MB = (21 << ZX_SYS_MAP_HUGE_SHIFT);
constexpr zx_i32 ZX_SYS_MAP_HUGE_1GB = (30 << ZX_SYS_MAP_HUGE_SHIFT);
constexpr zx_i32 ZX_SYS_MADV_HUGEPAGE = 14;

constexpr zx_i32 ZX_SYS_PROT_NONE = 0x0;
constexpr zx_i32 ZX_SYS_PROT_READ = 0x1;
constexpr zx_i32 ZX_SYS_PROT_WRITE = 0x2;

constexpr zx_i32 ZX_STDIN = 0;
constexpr zx_i32 ZX_STDOUT = 1;
constexpr zx_i32 ZX_STDERR = 2;

#endif

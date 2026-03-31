#ifndef ZX_SYS_H
#define ZX_SYS_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_def.h>
#include <zx/zx_err.h>

constexpr zx_i32 ZX_STDIN = 0;
constexpr zx_i32 ZX_STDOUT = 1;
constexpr zx_i32 ZX_STDERR = 2;

ZXStatus zx_sys_read(zx_i32 fd, zx_u8 *buf, zx_u64 size, zx_u64 *n_read,
                     ZXErrStack *err_stack);

ZXStatus zx_sys_write(zx_i32 fd, zx_u8 const *buf, zx_u64 size,
                      zx_u64 *n_written, ZXErrStack *err_stack);

ZXStatus zx_sys_open(char const *pathname, zx_i32 flags, zx_u32 mode,
                     zx_i32 *fd, ZXErrStack *err_stack);

ZXStatus zx_sys_close(zx_i32 fd, ZXErrStack *err_stack);

[[noreturn]]
void zx_sys_abort();

// returns the resulting pointer through `mem` argument
ZXStatus zx_sys_mmap(zx_u8 *address, zx_u64 length, zx_i32 prot, zx_i32 flags,
                     zx_i32 fd, zx_i64 offset, zx_u8 **mem,
                     ZXErrStack *err_stack);

ZXStatus zx_sys_munmap(zx_u8 *mem, zx_u64 length, ZXErrStack *err_stack);

ZXStatus zx_sys_madvise(zx_u8 *mem, zx_u64 length, zx_i32 advice,
                        ZXErrStack *err_stack);

ZXStatus zx_sys_mprotect(zx_u8 *mem, zx_u64 length, zx_i32 prot,
                         ZXErrStack *err_stack);

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

#endif

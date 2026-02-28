#ifndef ZX_SYS_H
#define ZX_SYS_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_def.h>

constexpr zx_i32 ZX_STDIN = 0;
constexpr zx_i32 ZX_STDOUT = 1;
constexpr zx_i32 ZX_STDERR = 2;

constexpr zx_usize ZX_SYS_STATUS_SUCCESS = 0;

typedef void zx_sys_sigactionfn(int signum);
zx_usize zx_sys_sigaction(int signum, zx_sys_sigactionfn actionfn);

zx_usize zx_sys_read(zx_i32 fd, void *buf, zx_usize size, zx_usize *n_read);

zx_usize zx_sys_write(zx_i32 fd, void const *buf, zx_usize size,
                      zx_usize *n_written);

[[noreturn]]
void zx_sys_abort(void);

[[noreturn]]
void zx_sys_exit(int code);

// returns the resulting pointer through `mem` argument
zx_usize zx_sys_mmap(void *address, zx_usize length, zx_i32 prot, zx_i32 flags,
                     zx_i32 fd, zx_i64 offset, void **mem);

zx_usize zx_sys_munmap(void *mem, zx_usize length);

zx_usize zx_sys_madvise(void *mem, zx_usize length, zx_i32 advice);

// echo '#include <sys/mman.h>' | gcc -E - -dM | grep "#define MAP_"
constexpr int ZX_SYS_MAP_PRIVATE = 0x02;
constexpr int ZX_SYS_MAP_ANONYMOUS = 0x20;
constexpr int ZX_SYS_MAP_LOCKED = 0x02000;
constexpr int ZX_SYS_MAP_POPULATE = 0x08000;
constexpr int ZX_SYS_MAP_HUGETLB = 0x40000;

constexpr int ZX_SYS_MAP_HUGE_SHIFT = 26;
constexpr int ZX_SYS_MAP_HUGE_2MB = (21 << ZX_SYS_MAP_HUGE_SHIFT);
constexpr int ZX_SYS_MAP_HUGE_1GB = (30 << ZX_SYS_MAP_HUGE_SHIFT);
constexpr int ZX_SYS_MADV_HUGEPAGE = 14;

constexpr int ZX_SYS_PROT_READ = 0x1;
constexpr int ZX_SYS_PROT_WRITE = 0x2;

// echo '#include <signal.h>' | gcc -E - -dM | grep SIGSEG
constexpr int ZX_SYS_SIGSEGV = 11;
constexpr int ZX_SYS_SIGABRT = 6;
constexpr int ZX_SYS_SIGFPE = 8;

constexpr int ZX_SYS_EXIT_SUCCESS = 0;
constexpr int ZX_SYS_EXIT_FAILURE = 1;

#endif

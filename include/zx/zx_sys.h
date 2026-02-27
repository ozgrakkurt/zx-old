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

zx_usize zx_sys_read(zx_i32 fd, void *buf, zx_usize size, zx_usize *n_read);

zx_usize zx_sys_write(zx_i32 fd, void const *buf, zx_usize size,
                      zx_usize *n_written);

[[noreturn]]
void zx_sys_abort(void);

// returns the resulting pointer through `mem` argument
zx_usize zx_sys_mmap(void *address, zx_usize length, zx_i32 prot, zx_i32 flags,
                     zx_i32 fd, zx_i64 offset, void **mem);

zx_usize zx_sys_munmap(void *mem, zx_usize length);

zx_usize zx_sys_madvise(void *mem, zx_usize length, zx_i32 advice);

#endif

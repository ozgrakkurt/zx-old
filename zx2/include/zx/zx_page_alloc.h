#ifndef ZX_PAGE_ALLOC_H
#define ZX_PAGE_ALLOC_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

#ifndef __linux__
#error "expected linux"
#endif

#ifndef __x86_64__
#error "expected x86_64"
#endif

constexpr zx_u64 ZX_PAGE_SIZE = 1 << 12;

ZXStatus zx_page_alloc_4kb(zx_u64 size, zx_u8 **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_thp_2mb(zx_u64 size, zx_u8 **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_huge_2mb(zx_u64 size, zx_u8 **mem,
                                ZXErrStack *err_stack);
ZXStatus zx_page_alloc_huge_1gb(zx_u64 size, zx_u8 **mem,
                                ZXErrStack *err_stack);
ZXStatus zx_page_alloc_free(zx_u8 *mem, zx_u64 size, ZXErrStack *err_stack);

#endif

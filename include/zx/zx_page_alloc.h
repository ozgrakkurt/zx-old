#ifndef ZX_PAGE_ALLOC_H
#define ZX_PAGE_ALLOC_H

#include <zx/zx_def.h>
#include <zx/zx_err.h>

ZXStatus zx_page_alloc_4kb(zx_u64 size, void **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_thp_2mb(zx_u64 size, void **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_huge_2mb(zx_u64 size, void **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_huge_1gb(zx_u64 size, void **mem, ZXErrStack *err_stack);
ZXStatus zx_page_alloc_free(void *mem, zx_u64 size, ZXErrStack *err_stack);

#endif

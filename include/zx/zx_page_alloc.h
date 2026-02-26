#ifndef ZX_PAGE_ALLOC_H
#define ZX_PAGE_ALLOC_H

#include <zx/zx_def.h>

void *zx_page_alloc_4kb(zx_usize size);
void *zx_page_alloc_thp_2mb(zx_usize size);
void *zx_page_alloc_huge_2mb(zx_usize size);
void *zx_page_alloc_huge_1gb(zx_usize size);
void zx_page_alloc_free(void *page, zx_usize size);

#endif

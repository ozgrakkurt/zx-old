#ifndef SHIN_PAGE_ALLOC_H
#define SHIN_PAGE_ALLOC_H

#include <shin/shin_def.h>

void *shin_page_alloc_4kb(shin_usize size);
void *shin_page_alloc_thp_2mb(shin_usize size);
void *shin_page_alloc_huge_2mb(shin_usize size);
void *shin_page_alloc_huge_1gb(shin_usize size);
void shin_page_alloc_free(void *page, shin_usize size);

#endif

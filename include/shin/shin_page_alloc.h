#ifndef SHIN_PAGE_ALLOC_H
#define SHIN_PAGE_ALLOC_H

#include <shin/shin_def.h>

void *shin_page_alloc_4kb(shin_usize size);
void *shin_page_alloc_thp_2mb(shin_usize size);
void *shin_page_alloc_huge_2mb(shin_usize size);
void *shin_page_alloc_huge_1gb(shin_usize size);
void shin_page_alloc_free(void *page, shin_usize size);

// memlock all current and future allocations into memory.
// useful for makign sure the executable section of the program is pinned to
// memory.
void shin_page_alloc_memlock_all(void);

#endif

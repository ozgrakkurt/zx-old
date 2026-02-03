#ifndef SHIN_ALLOC_H
#define SHIN_ALLOC_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>

#ifdef __cplusplus
extern "C" {
#endif

ShinStatus shin_page_alloc_thp(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack);
ShinStatus shin_page_alloc_2mb(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack);
ShinStatus shin_page_alloc_1gb(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack);
ShinStatus shin_page_free(void* page, shin_usize size, ShinErrStack* err_stack);

#ifdef __cplusplus
}
#endif

#endif

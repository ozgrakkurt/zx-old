#ifndef BLIB_PAGE_ALLOC_HPP
#define BLIB_PAGE_ALLOC_HPP

#include <blib_err.hpp>
#include <cstddef>

namespace blib {

Status page_alloc_thp(size_t size, void** page_ptr);
Status page_alloc_2MB(size_t size, void** page_ptr);
Status page_alloc_1GB(size_t size, void** page_ptr);
Status page_free(void* page, size_t size);

}  // namespace blib

#endif

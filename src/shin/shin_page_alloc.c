#include <shin/shin_def.h>
#include <shin/shin_page_alloc.h>
#include <stddef.h>

#include <sys/mman.h>

static ShinStatus shin_page_mmap(shin_usize size, void** page_ptr,
                                 int huge_page_flags, ShinErrStack* err_stack) {
  int const prot = PROT_READ | PROT_WRITE;
  int const flags =
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_POPULATE | huge_page_flags;

  void* const res = mmap(NULL, (size_t){size}, prot, flags, -1, 0);
  if (res == MAP_FAILED) {
    *page_ptr = nullptr;
    SHIN_ERR_RETURN(ShinStatus_OutOfMemory, err_stack);
  }

  *page_ptr = res;

  return ShinStatus_Ok;
}

ShinStatus shin_page_alloc(shin_usize size, void** page_ptr,
                           ShinErrStack* err_stack) {
  constexpr shin_usize ALIGNMENT = 1 << 12;

  SHIN_ERR_ASSERT((size > 0), err_stack);
  SHIN_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  SHIN_ERR_TRY((shin_page_mmap(size, page_ptr, 0, err_stack)), err_stack);

  return ShinStatus_Ok;
}

ShinStatus shin_page_alloc_thp(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack) {
  constexpr shin_usize ALIGNMENT = 1 << 21;

  SHIN_ERR_ASSERT((size > 0), err_stack);
  SHIN_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  SHIN_ERR_TRY((shin_page_mmap(size, page_ptr, 0, err_stack)), err_stack);

  void* const page = *page_ptr;

  if (-1 == madvise(page, (size_t){size}, MADV_HUGEPAGE)) {
    if (-1 == munmap(page, (size_t){size})) {
      // just leak memory since we can't do much about it at this point
      // at least seperate error return here so we can see this specific path
      // was taken in the stack trace.
      *page_ptr = nullptr;
      SHIN_ERR_RETURN((ShinStatus_UnexpectedError), err_stack);
    }
    *page_ptr = nullptr;
    SHIN_ERR_RETURN((ShinStatus_UnexpectedError), err_stack);
  }

  return ShinStatus_Ok;
}

ShinStatus shin_page_alloc_2MB(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack) {
  constexpr shin_usize ALIGNMENT = 1 << 21;

  SHIN_ERR_ASSERT((size > 0), err_stack);
  SHIN_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  SHIN_ERR_TRY(
      (shin_page_mmap(size, page_ptr, MAP_HUGETLB | MAP_HUGE_2MB, err_stack)),
      err_stack);

  return ShinStatus_Ok;
}

ShinStatus shin_page_alloc_1GB(shin_usize size, void** page_ptr,
                               ShinErrStack* err_stack) {
  constexpr shin_usize ALIGNMENT = 1 << 30;

  SHIN_ERR_ASSERT((size > 0), err_stack);
  SHIN_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  SHIN_ERR_TRY(
      (shin_page_mmap(size, page_ptr, MAP_HUGETLB | MAP_HUGE_1GB, err_stack)),
      err_stack);

  return ShinStatus_Ok;
}

ShinStatus shin_page_free(void* page, shin_usize size,
                          ShinErrStack* err_stack) {
  if (-1 == munmap(page, (size_t){size})) {
    SHIN_ERR_RETURN(ShinStatus_UnexpectedError, err_stack);
  }
  return ShinStatus_Ok;
}

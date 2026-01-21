#include <sys/mman.h>

#include <blib_err.hpp>
#include <blib_page_alloc.hpp>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace blib {

static Status page_mmap(size_t size, void** page_ptr, int huge_page_flags) {
  int const prot = PROT_READ | PROT_WRITE;
  int const flags =
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_POPULATE | huge_page_flags;

  void* const res = mmap(NULL, size, prot, flags, -1, 0);
  if (res == MAP_FAILED) {
    *page_ptr = nullptr;
    BLIB_ERR_TRY((Status::OutOfMemory));
  }

  *page_ptr = res;

  return Status::Ok;
}

Status page_alloc_thp(size_t size, void** page_ptr) {
  size_t constexpr ALIGNMENT = 1 << 21;

  BLIB_ERR_ASSERT((size > 0));
  BLIB_ERR_ASSERT((size % ALIGNMENT == 0));

  BLIB_ERR_TRY((page_mmap(size, page_ptr, 0)));

  void* const page = *page_ptr;

  if (-1 == madvise(page, size, MADV_HUGEPAGE)) {
    if (-1 == munmap(page, size)) {
      fprintf(
          stderr,
          "failed to munmap after madvise failed in blib::page_alloc_thp.\n");
      abort();
    }
    *page_ptr = nullptr;
    BLIB_ERR_TRY((Status::UnexpectedError));
  }

  return Status::Ok;
}

Status page_alloc_2MB(size_t size, void** page_ptr) {
  size_t constexpr ALIGNMENT = 1 << 21;

  BLIB_ERR_ASSERT((size > 0));
  BLIB_ERR_ASSERT((size % ALIGNMENT == 0));

  BLIB_ERR_TRY((page_mmap(size, page_ptr, MAP_HUGETLB | MAP_HUGE_2MB)));

  return Status::Ok;
}

Status page_alloc_1GB(size_t size, void** page_ptr) {
  size_t constexpr ALIGNMENT = 1 << 30;

  BLIB_ERR_ASSERT((size > 0));
  BLIB_ERR_ASSERT((size % ALIGNMENT == 0));

  BLIB_ERR_TRY((page_mmap(size, page_ptr, MAP_HUGETLB | MAP_HUGE_1GB)));

  return Status::Ok;
}

Status page_free(void* page, size_t size) {
  if (-1 == munmap(page, size)) {
    BLIB_ERR_TRY((Status::UnexpectedError));
  }
  return Status::Ok;
}

}  // namespace blib

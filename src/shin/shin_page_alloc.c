#include <shin/shin_def.h>
#include <shin/shin_page_alloc.h>
#include <shin/shin_err.h>
#include <stddef.h>
#include <sys/mman.h>

static void* shin_page_mmap(shin_usize size, int huge_page_flags) {
  int const prot = PROT_READ | PROT_WRITE;
  int const flags =
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_POPULATE | huge_page_flags;

  void* const res = mmap(nullptr, (size_t){size}, prot, flags, -1, 0);
  SHIN_ASSERT((res != MAP_FAILED));

  return res;
}

void* shin_page_alloc_4kb(shin_usize size) {
  constexpr shin_usize ALIGNMENT = 1 << 12;

  SHIN_ASSERT((size > 0));
  SHIN_ASSERT((size % ALIGNMENT == 0));

  return shin_page_mmap(size, 0);
}

void* shin_page_alloc_thp_2mb(shin_usize size) {
  constexpr shin_usize ALIGNMENT = 1 << 21;

  SHIN_ASSERT((size > 0));
  SHIN_ASSERT((size % ALIGNMENT == 0));

  void* const page = shin_page_mmap(size, 0);

  SHIN_ASSERT((-1 != madvise(page, (size_t){size}, MADV_HUGEPAGE)));

  return page;
}

void* shin_page_alloc_huge_2mb(shin_usize size) {
  constexpr shin_usize ALIGNMENT = 1 << 21;

  SHIN_ASSERT((size > 0));
  SHIN_ASSERT((size % ALIGNMENT == 0));

  return shin_page_mmap(size, MAP_HUGETLB | MAP_HUGE_2MB);
}

void* shin_page_alloc_huge_1gb(shin_usize size) {
  constexpr shin_usize ALIGNMENT = 1 << 30;

  SHIN_ASSERT((size > 0));
  SHIN_ASSERT((size % ALIGNMENT == 0));

  return shin_page_mmap(size, MAP_HUGETLB | MAP_HUGE_1GB);
}

void shin_page_free(void* page, shin_usize size) {
  SHIN_ASSERT((-1 != munmap(page, (size_t){size})));
}

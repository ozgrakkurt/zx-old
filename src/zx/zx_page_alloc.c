#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

// echo '#include <sys/mman.h>' | gcc -E - -dM | grep "#define MAP_"
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20
#define MAP_LOCKED 0x02000
#define MAP_POPULATE 0x08000
#define MAP_HUGETLB 0x40000

#define MAP_HUGE_SHIFT 26
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT)
#define MADV_HUGEPAGE 14

#define PROT_READ 0x1
#define PROT_WRITE 0x2

static void *zx_page_mmap(zx_usize size, int huge_page_flags) {
  int const prot = PROT_READ | PROT_WRITE;
  int const flags =
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED | MAP_POPULATE | huge_page_flags;

  void *mem = nullptr;
  zx_usize const status = zx_sys_mmap(nullptr, size, prot, flags, -1, 0, &mem);
  ZX_ASSERT((status == ZX_SYS_STATUS_SUCCESS));
  ZX_ASSERT((mem != nullptr));

  return mem;
}

void *zx_page_alloc_4kb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 12;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  return zx_page_mmap(size, 0);
}

void *zx_page_alloc_thp_2mb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 21;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  void *const page = zx_page_mmap(size, 0);

  ZX_ASSERT(
      (ZX_SYS_STATUS_SUCCESS == zx_sys_madvise(page, size, MADV_HUGEPAGE)));

  return page;
}

void *zx_page_alloc_huge_2mb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 21;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  return zx_page_mmap(size, MAP_HUGETLB | MAP_HUGE_2MB);
}

void *zx_page_alloc_huge_1gb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 30;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  return zx_page_mmap(size, MAP_HUGETLB | MAP_HUGE_1GB);
}

void zx_page_alloc_free(void *page, zx_usize size) {
  ZX_ASSERT((page != nullptr));
  ZX_ASSERT((ZX_SYS_STATUS_SUCCESS == zx_sys_munmap(page, size)));
}

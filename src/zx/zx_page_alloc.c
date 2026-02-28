#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

static void *zx_page_mmap(zx_usize size, int huge_page_flags) {
  int const prot = ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE;
  int const flags = ZX_SYS_MAP_PRIVATE | ZX_SYS_MAP_ANONYMOUS |
                    ZX_SYS_MAP_LOCKED | ZX_SYS_MAP_POPULATE | huge_page_flags;

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

  ZX_ASSERT((ZX_SYS_STATUS_SUCCESS ==
             zx_sys_madvise(page, size, ZX_SYS_MADV_HUGEPAGE)));

  return page;
}

void *zx_page_alloc_huge_2mb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 21;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  return zx_page_mmap(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_2MB);
}

void *zx_page_alloc_huge_1gb(zx_usize size) {
  constexpr zx_usize ALIGNMENT = 1 << 30;

  ZX_ASSERT((size > 0));
  ZX_ASSERT((size >= ALIGNMENT));
  ZX_ASSERT((size % ALIGNMENT == 0));

  return zx_page_mmap(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_1GB);
}

void zx_page_alloc_free(void *page, zx_usize size) {
  ZX_ASSERT((page != nullptr));
  ZX_ASSERT((ZX_SYS_STATUS_SUCCESS == zx_sys_munmap(page, size)));
}

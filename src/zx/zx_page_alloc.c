#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_sys.h>

static ZXStatus zx_page_mmap(zx_u64 size, int huge_page_flags, zx_u8 **mem,
                             ZXErrStack *err_stack) {
  ZX_ERR_ASSERT((mem != nullptr), err_stack);
  ZX_ERR_ASSERT((*mem == nullptr), err_stack);

  int const prot = ZX_SYS_PROT_READ | ZX_SYS_PROT_WRITE;
  int const flags = ZX_SYS_MAP_PRIVATE | ZX_SYS_MAP_ANONYMOUS |
                    ZX_SYS_MAP_LOCKED | ZX_SYS_MAP_POPULATE | huge_page_flags;

  ZX_ERR_TRY((zx_sys_mmap(nullptr, size, prot, flags, -1, 0, mem, err_stack)),
             err_stack);
  ZX_ASSERT((*mem != nullptr));

  return ZXStatus_Ok;
}

ZXStatus zx_page_alloc_4kb(zx_u64 size, zx_u8 **mem, ZXErrStack *err_stack) {
  constexpr zx_u64 ALIGNMENT = 1 << 12;

  ZX_ERR_ASSERT((size > 0), err_stack);
  ZX_ERR_ASSERT((size >= ALIGNMENT), err_stack);
  ZX_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  ZX_ERR_TRY((zx_page_mmap(size, 0, mem, err_stack)), err_stack);

  return ZXStatus_Ok;
}

ZXStatus zx_page_alloc_thp_2mb(zx_u64 size, zx_u8 **mem,
                               ZXErrStack *err_stack) {
  constexpr zx_u64 ALIGNMENT = 1 << 21;

  ZX_ERR_ASSERT((size > 0), err_stack);
  ZX_ERR_ASSERT((size >= ALIGNMENT), err_stack);
  ZX_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  ZX_ERR_TRY((zx_page_mmap(size, 0, mem, err_stack)), err_stack);

  ZXStatus const madvise_status =
      zx_sys_madvise(*mem, size, ZX_SYS_MADV_HUGEPAGE, err_stack);
  if (madvise_status != ZXStatus_Ok) {
    ZX_ERR_TRY((zx_page_alloc_free(*mem, size, err_stack)), err_stack);
    ZX_ERR_TRY((madvise_status), err_stack);
  }

  return ZXStatus_Ok;
}

ZXStatus zx_page_alloc_huge_2mb(zx_u64 size, zx_u8 **mem,
                                ZXErrStack *err_stack) {
  constexpr zx_u64 ALIGNMENT = 1 << 21;

  ZX_ERR_ASSERT((size > 0), err_stack);
  ZX_ERR_ASSERT((size >= ALIGNMENT), err_stack);
  ZX_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  ZX_ERR_TRY((zx_page_mmap(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_2MB, mem,
                           err_stack)),
             err_stack);

  return ZXStatus_Ok;
}

ZXStatus zx_page_alloc_huge_1gb(zx_u64 size, zx_u8 **mem,
                                ZXErrStack *err_stack) {
  constexpr zx_u64 ALIGNMENT = 1 << 30;

  ZX_ERR_ASSERT((size > 0), err_stack);
  ZX_ERR_ASSERT((size >= ALIGNMENT), err_stack);
  ZX_ERR_ASSERT((size % ALIGNMENT == 0), err_stack);

  ZX_ERR_TRY((zx_page_mmap(size, ZX_SYS_MAP_HUGETLB | ZX_SYS_MAP_HUGE_1GB, mem,
                           err_stack)),
             err_stack);

  return ZXStatus_Ok;
}

ZXStatus zx_page_alloc_free(zx_u8 *page, zx_u64 size, ZXErrStack *err_stack) {
  ZX_ERR_ASSERT((page != nullptr), err_stack);

  ZX_ERR_TRY((zx_sys_munmap(page, size, err_stack)), err_stack);

  return ZXStatus_Ok;
}

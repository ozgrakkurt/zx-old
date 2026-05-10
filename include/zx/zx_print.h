#ifndef ZX_PRINT_H
#define ZX_PRINT_H

#include <zx/zx_fmt.h>
#include <zx/zx_sys.h>

#define ZX_EPRINT(...)                                                         \
  do {                                                                         \
    constexpr zx_u64 zx_p_arr_cap_ = 1 << 12;                                  \
    zx_u8 zx_p_arr_[zx_p_arr_cap_];                                            \
    ZXSliceMut<zx_u8> zx_p_mem_ =                                              \
        ZXSliceMut<zx_u8>::init_unsafe(zx_p_arr_, zx_p_arr_cap_);              \
    ZXFmt zx_p_fmt_ = ZXFmt::init(zx_p_mem_);                                  \
    bool zx_p_overflow_ = false;                                               \
    ZX_FMT(&zx_p_fmt_, &zx_p_overflow_, __VA_ARGS__);                          \
    (void)zx_p_overflow_;                                                      \
    zx_u64 zx_p_n_written_ = 0;                                                \
    (void)zx_sys_write(ZX_STDERR, zx_p_fmt_.written(), &zx_p_n_written_);      \
    (void)zx_p_n_written_;                                                     \
  } while (false)

#endif

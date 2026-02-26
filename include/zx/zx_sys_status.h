#ifndef ZX_SYS_STATUS_H
#define ZX_SYS_STATUS_H

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

constexpr zx_usize ZX_SYS_STATUS_SUCCESS = 0;

#endif

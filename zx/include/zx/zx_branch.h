#ifndef ZX_BRANCH_H
#define ZX_BRANCH_H

#define zx_likely(x) __builtin_expect(!!(x), 1)
#define zx_unlikely(x) __builtin_expect(!!(x), 0)

#endif

// This code is mostly taken from Rust stdlib sort implementation.
//
// Orignal code is available under the MIT license:
// Copyright (c) The Rust Project Contributors
//
// Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the
// Software without restriction, including without
// limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ZX_IPNSORT_H
#define ZX_IPNSORT_H

#include <zx/zx_int.h>
#include <zx/zx_slice.h>

constexpr zx_u64 ZX_IPNSORT_INSERTION_SORT_THRESHOLD = 20;

struct ZXIpnSortFindExistingRunResult {
  zx_u64 run_len;
  bool is_strictly_descending;
};

template <typename T, typename Ctx,
          bool IsLess(Ctx ctx, T *const left, T *const right)>
static inline ZXIpnSortFindExistingRunResult
zx_ipnsort_find_existing_run_(ZXSlice<T> slice, Ctx ctx) {
  if (slice.len() < 2) {
    return ZXIpnSortFindExistingRunResult{
        .run_len = slice.len(),
        .is_strictly_descending = false,
    };
  }

  zx_u64 run_len = 2;
  bool const strictly_descending =
      IsLess(ctx, slice.offset_unsafe(1), slice.offset_unsafe(0));
  if (strictly_descending) {
    while (run_len < slice.len() &&
           IsLess(ctx, slice.offset_unsafe(run_len),
                  slice.offset_unsafe(zx_sub_unsafe_u64(run_len, 1)))) {
      run_len = zx_add_unsafe_u64(run_len, 1);
    }
  } else {
    while (run_len < slice.len() &&
           !IsLess(ctx, slice.offset_unsafe(run_len),
                   slice.offset_unsafe(zx_sub_unsafe_u64(run_len, 1)))) {
      run_len = zx_add_unsafe_u64(run_len, 1);
    }
  }

  return ZXIpnSortFindExistingRunResult{
      .run_len = run_len,
      .is_strictly_descending = strictly_descending,
  };
}

template <typename T, typename Ctx,
          bool IsLess(Ctx ctx, T *const left, T *const right)>
static inline void zx_ipnsort_insertion_sort_(ZXSliceMut<T> slice, Ctx ctx) {
  if (slice.len() == 0) {
    return;
  }

#pragma clang loop unroll(disable)
  for (zx_u64 i = 1; i < slice.len(); i++) {
    T const pivot = slice.read_unsafe(i);
    zx_u64 j = i;
    while (j > 0) {
      zx_u64 const prev_idx = zx_sub_unsafe_u64(j, 1);
      T *prev = slice.offset_unsafe(prev_idx);
      T *curr = slice.offset_unsafe(j);
      if (IsLess(ctx, curr, prev)) {
        *curr = *prev;
        j = prev_idx;
      } else {
        break;
      }
    }
    slice.write_unsafe(j, pivot);
  }
}

template <typename T, typename Ctx,
          bool IsLess(Ctx ctx, T *const left, T *const right)>
static inline void zx_ipnsort_quicksort_(ZXSliceMut<T> slice, zx_u64 limit,
                                         Ctx ctx) {
  while (true) {
    if (slice.len() <= ZX_IPNSORT_INSERTION_SORT_THRESHOLD) {
      zx_ipnsort_insertion_sort_(slice, ctx);
    }

    if (limit == 0) {
    }
  }
}

template <typename T, typename Ctx,
          bool IsLess(Ctx ctx, T *const left, T *const right)>
__attribute__((noinline)) static inline void
zx_ipnsort_impl_(ZXSliceMut<T> slice, Ctx ctx) {
  ZXIpnSortFindExistingRunResult const existing_run =
      zx_ipnsort_find_existing_run_<T, Ctx, IsLess>(slice.as_const(), ctx);

  if (existing_run.run_len == slice.len()) {
    if (existing_run.is_strictly_descending) {
      slice.reverse();
    }

    return;
  }

  zx_u64 const limit =
      zx_mul_unsafe_u64(2, zx_ilog2_u64(slice.len() | zx_u64{1__wbu}));

  zx_ipnsort_quicksort_<T, Ctx, IsLess>(slice, limit, ctx);
}

template <typename T, typename Ctx,
          bool IsLess(Ctx ctx, T *const left, T *const right)>
static inline void zx_ipnsort(ZXSliceMut<T> slice, Ctx ctx) {
  if (slice.len() < 2) [[likely]] {
    return;
  }

  if (slice.len() <= ZX_IPNSORT_INSERTION_SORT_THRESHOLD) [[likely]] {
    zx_ipnsort_insertion_sort_<T, Ctx, IsLess>(slice, ctx);
  } else {
    zx_ipnsort_impl_<T, Ctx, IsLess>(slice, ctx);
  }
}

#endif

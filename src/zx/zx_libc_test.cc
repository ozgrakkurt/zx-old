#include "./zx_libc.h"

static void assert(bool test) {
  if (!test) {
    __builtin_trap();
  }
}

__attribute__((no_builtin)) static inline void check_memcpy(zx_u8* dst,
                                                            zx_u8* src,
                                                            zx_u64 count) {
  for (zx_u64 i = 0; i < count; i++) {
    src[i] = (zx_u8)i;
    dst[i] = 69;
  }

  void* out_dst = zx_memcpy(dst, src, count);
  assert(out_dst == dst);

  for (zx_u64 i = 0; i < count; i++) {
    assert(src[i] == dst[i]);
  }
}

__attribute__((no_builtin)) static inline void check_memset(zx_u8* dst,
                                                            zx_u64 count) {
  for (zx_u64 i = 0; i < count; i++) {
    dst[i] = (zx_u8)i;
  }

  void* out_dst = zx_memset(dst, 69, count);
  assert(out_dst == dst);

  for (zx_u64 i = 0; i < count; i++) {
    assert(dst[i] == 69);
  }
}

__attribute__((no_builtin)) static inline void check_memcmp(zx_u8* left,
                                                            zx_u8* right,
                                                            zx_u64 count) {
  // this should have a fuzz test!
  // diff pos first, last, middle. put some diffs after the first diff too
  // (after 5 chars?). lt, gt, eq

  for (zx_u64 i = 0; i < count; i++) {
    right[i] = (zx_u8)i;
    left[i] = (zx_u8)i;
  }

  zx_i32 const res = zx_memcmp(left, right, count);
  assert(res == 0);
}

int main(void) {
  constexpr zx_u64 capacity = 1024;
  zx_u8 src[capacity];
  zx_u8 dst[capacity];

  constexpr zx_u64 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129};
  constexpr zx_u64 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u64 i = 0; i < n_lengths; i++) {
    check_memcpy(dst, src, lengths[i]);
    check_memset(dst, lengths[i]);
    check_memcmp(src, dst, lengths[i]);
  }

  return 0;
}

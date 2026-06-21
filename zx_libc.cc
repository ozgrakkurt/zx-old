typedef unsigned _BitInt(8) zx_u8;
typedef unsigned _BitInt(16) zx_u16;
typedef unsigned _BitInt(32) zx_u32;
typedef unsigned _BitInt(64) zx_u64;
typedef unsigned _BitInt(128) zx_u128;
typedef unsigned _BitInt(256) zx_u256;
typedef _BitInt(8) zx_i8;
typedef _BitInt(16) zx_i16;
typedef _BitInt(32) zx_i32;
typedef _BitInt(64) zx_i64;
typedef _BitInt(128) zx_i128;
typedef _BitInt(256) zx_i256;

#ifdef ZX_DEV
static inline void zx_assert_dev(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}
#else
static inline void zx_assert_dev(bool cond) {
  (void)cond;
}
#endif



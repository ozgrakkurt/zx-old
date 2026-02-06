#ifndef SHIN_DEF_H
#define SHIN_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define SHIN_ASSERT_SAME_TYPE(a, b)                                            \
  _Static_assert(_Generic((a), typeof_unqual(b): 1, default: 0),               \
                 "Types do not "                                               \
                 "match!")

#define SHIN_UINT_MAX(T) (~((T){0wbu}))
#define SHIN_INT_MAX(T) ((T){((~((T){0wb})) >> 1)})
#define SHIN_INT_MIN(T) (~SHIN_INT_MAX(T))

#define SHIN_MIN(A, B)                                                         \
  ({                                                                           \
    auto const a = (A);                                                        \
    auto const b = (B);                                                        \
                                                                               \
    SHIN_ASSERT_SAME_TYPE(a, b);                                               \
                                                                               \
    a < b ? a : b;                                                             \
  })

#define SHIN_MAX(A, B)                                                         \
  ({                                                                           \
    auto const a = (A);                                                        \
    auto const b = (B);                                                        \
                                                                               \
    SHIN_ASSERT_SAME_TYPE(a, b);                                               \
                                                                               \
    a > b ? a : b;                                                             \
  })

typedef unsigned _BitInt(8) shin_u8;
typedef unsigned _BitInt(16) shin_u16;
typedef unsigned _BitInt(32) shin_u32;
typedef unsigned _BitInt(64) shin_u64;
typedef unsigned _BitInt(128) shin_u128;
typedef unsigned _BitInt(256) shin_u256;
typedef unsigned _BitInt(sizeof(void *) * 8) shin_usize;

_Static_assert(sizeof(shin_usize) == sizeof(void *));

typedef _BitInt(8) shin_i8;
typedef _BitInt(16) shin_i16;
typedef _BitInt(32) shin_i32;
typedef _BitInt(64) shin_i64;
typedef _BitInt(128) shin_i128;
typedef _BitInt(256) shin_i256;
typedef _BitInt(sizeof(void *) * 8) shin_isize;

_Static_assert(sizeof(shin_isize) == sizeof(void *));

typedef _Float16 shin_f16;
typedef float shin_f32;
typedef double shin_f64;

_Static_assert(sizeof(shin_f16) == 2);
_Static_assert(sizeof(shin_f32) == 4);
_Static_assert(sizeof(shin_f64) == 8);

#ifdef __cplusplus
}
#endif

#endif

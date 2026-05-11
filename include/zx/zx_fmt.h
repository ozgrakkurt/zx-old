#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_assert.h>
#include <zx/zx_int.h>
#include <zx/zx_panic.h>
#include <zx/zx_slice.h>

struct ZXFmt {
  // Backing memory
  ZXSliceMut<zx_u8> m_mem;
  // Current write offset;
  zx_u64 m_offset;

  // Create a new formatter with given memory. The formatter will
  // use this memory for writing the characters.
  static inline ZXFmt init(ZXSliceMut<zx_u8> mem) {
    return ZXFmt{
        .m_mem = mem,
        .m_offset = 0,
    };
  }

  inline zx_u64 n_written() const { return this->m_offset; }

  inline zx_u64 capacity() const { return this->m_mem.len(); }

  // Return a slice that points to the characters written so far.
  inline ZXSlice<zx_u8> written() const {
    return this->m_mem.slice_unsafe(0, this->m_offset).as_const();
  }

  inline void reset() {
    this->m_mem.fill(zx_u8{0xaa});
    this->m_offset = 0;
  }

  inline void reset_unsafe() {
#ifdef ZX_DEV
    this->m_mem.fill(zx_u8{0xaa});
#endif
    this->m_offset = 0;
  }

  inline void push_str(ZXSlice<zx_u8> str, bool *overflow) {
    ZXSliceMut<zx_u8> const this_mem = this->m_mem;
    zx_u64 const this_offset = this->m_offset;
    zx_u64 const capacity_left = zx_sub_unsafe_u64(this_mem.len(), this_offset);
    zx_u64 const n_write = zx_min_u64(capacity_left, str.len());
    if (n_write < str.len()) {
      if (overflow != nullptr) {
        *overflow = true;
      } else {
#ifdef ZX_DEV
        zx_panic();
#endif
      }
    }
    ZXSliceMut<zx_u8> const w_slice =
        this_mem.slice_unsafe(this_offset, n_write);
    ZXSlice<zx_u8> const w_data = str.slice_unsafe(0, n_write);
    w_slice.memcpy_from_unsafe(w_data);
    this->m_offset = zx_add_unsafe_u64(n_write, this_offset);
  }

  inline void push_cstring(ZXCString cstr, bool *overflow) {
    this->push_str(cstr.slice(), overflow);
  }

  inline void push_char(zx_u8 ch, bool *overflow) {
    ZXSliceMut<zx_u8> const this_mem = this->m_mem;
    zx_u64 const this_offset = this->m_offset;
    zx_u64 const capacity_left = zx_sub_unsafe_u64(this_mem.len(), this_offset);
    if (capacity_left > 0) {
      this_mem.write_unsafe(this_offset, ch);
      this->m_offset = zx_add_unsafe_u64(this_offset, 1);
    } else {
      if (overflow != nullptr) {
        *overflow = true;
      } else {
#ifdef ZX_DEV
        zx_panic();
#endif
      }
    }
  }

  inline void push_u64(zx_u64 val, bool *overflow) {
    constexpr zx_u64 MAX_NUM_DIGITS = 20;
    zx_u8 digits[MAX_NUM_DIGITS];

    zx_u64 pos = MAX_NUM_DIGITS;
    do {
      pos--;
      zx_assert_dev(pos < MAX_NUM_DIGITS);
      digits[pos] = (zx_u8)'0' + (zx_u8)(val % 10);
      val /= 10;
    } while (val > 0);
    zx_u64 const len = zx_sub_unsafe_u64(MAX_NUM_DIGITS, pos);
    ZXSlice<zx_u8> const str = ZXSlice<zx_u8>::init_unsafe(&digits[pos], len);
    this->push_str(str, overflow);
  }

  inline void push_i64(zx_i64 val, bool *overflow) {
    if (val >= 0) {
      this->push_u64((zx_u64)val, overflow);
    } else {
      ZXSliceMut<zx_u8> const this_mem = this->m_mem;
      zx_u64 const this_offset = this->m_offset;
      zx_u64 const capacity_left =
          zx_sub_unsafe_u64(this_mem.len(), this_offset);
      if (capacity_left > 0) {
        this_mem.write_unsafe(this_offset, zx_u8{'-'});
        this->m_offset = zx_add_unsafe_u64(this_offset, 1);
        this->push_u64((zx_u64)-val, overflow);
      } else {
        if (overflow != nullptr) {
          *overflow = true;
        } else {
#ifdef ZX_DEV
          zx_panic();
#endif
        }
      }
    }
  }

  inline void push_u64_hex(zx_u64 val, bool *overflow) {
    constexpr zx_u64 MAX_NUM_DIGITS = 22;
    zx_u8 digits[MAX_NUM_DIGITS];

    constexpr zx_u8 TABLE[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    zx_u64 pos = MAX_NUM_DIGITS;
    do {
      pos = zx_sub_unsafe_u64(pos, 1);
      digits[pos] = TABLE[val % 16];
      val /= 16;
    } while (val > 0);

    pos = zx_sub_unsafe_u64(pos, 1);
    digits[pos] = 'x';
    pos = zx_sub_unsafe_u64(pos, 1);
    digits[pos] = '0';

    zx_u64 const len = zx_sub_unsafe_u64(MAX_NUM_DIGITS, pos);
    ZXSlice<zx_u8> const str = ZXSlice<zx_u8>::init_unsafe(&digits[pos], len);
    this->push_str(str, overflow);
  }

  // Dummy func so we get a compile error if formatting isn't implemented for
  // `T`
  template <typename T> inline void format(T val, bool *overflow) {
    static_assert(sizeof(T) == 0);
    (void)val;
    (void)overflow;
  }

  template <>
  inline void format<char const *>(char const *cstr, bool *overflow) {
    this->push_cstring(ZXCString::init_unsafe(cstr), overflow);
  }

  template <> inline void format<char>(char ch, bool *overflow) {
    this->push_char((zx_u8)ch, overflow);
  }

  template <> inline void format<zx_u64>(zx_u64 val, bool *overflow) {
    this->push_u64(val, overflow);
  }

  template <> inline void format<zx_u8>(zx_u8 val, bool *overflow) {
    this->format<zx_u64>(val, overflow);
  }

  template <> inline void format<zx_u16>(zx_u16 val, bool *overflow) {
    this->format<zx_u64>(val, overflow);
  }

  template <> inline void format<zx_u32>(zx_u32 val, bool *overflow) {
    this->format<zx_u64>(val, overflow);
  }

  template <> inline void format<zx_i64>(zx_i64 val, bool *overflow) {
    this->push_i64(val, overflow);
  }

  template <> inline void format<zx_i8>(zx_i8 val, bool *overflow) {
    this->format<zx_i64>(val, overflow);
  }

  template <> inline void format<zx_i16>(zx_i16 val, bool *overflow) {
    this->format<zx_i64>(val, overflow);
  }

  template <> inline void format<zx_i32>(zx_i32 val, bool *overflow) {
    this->format<zx_i64>(val, overflow);
  }
};

#define ZX_FMT_1_(fmt, overflow, v) (fmt)->format((v), (overflow))
#define ZX_FMT_2_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_1_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_3_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_2_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_4_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_3_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_5_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_4_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_6_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_5_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_7_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_6_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_8_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_7_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_9_(fmt, overflow, v, ...)                                       \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_8_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_10_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_9_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_11_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_10_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_12_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_11_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_13_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_12_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_14_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_13_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_15_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_14_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT_16_(fmt, overflow, v, ...)                                      \
  (fmt)->format((v), (overflow))                                               \
      __VA_OPT__(; ZX_FMT_15_(fmt, overflow, __VA_ARGS__))
#define ZX_FMT(fmt, overflow, v, ...) ZX_FMT_16_(fmt, overflow, v, __VA_ARGS__)

#endif

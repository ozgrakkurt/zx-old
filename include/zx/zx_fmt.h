#ifndef ZX_FMT_H
#define ZX_FMT_H

#include <zx/zx_int.h>
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
    this->m_mem.fill(zx_u8{0});
    this->m_offset = 0;
  }

  inline void reset_unsafe() {
#ifdef ZX_DEV
    this->m_mem.fill(zx_u8{0});
#endif
    this->m_offset = 0;
  }

  inline void push_str(ZXSlice<zx_u8> str, bool* overflow) {
    ZXSliceMut<zx_u8> const this_mem = this->m_mem;
    zx_u64 const this_offset = this->m_offset;
    zx_u64 const capacity_left = zx_sub_unsafe_u64(this_mem.len(), this_offset);
    zx_u64 const n_write = zx_min_u64(capacity_left, str.len());
    if (n_write < str.len()) {
      if (overflow != nullptr) {
        *overflow = true;
      } else {
#ifdef ZX_DEV
        __builtin_trap();
#endif
      }
    }
    ZXSliceMut<zx_u8> const w_slice =
        this_mem.slice_unsafe(this_offset, n_write);
    ZXSlice<zx_u8> const w_data = str.slice_unsafe(0, n_write);
    w_slice.memcpy_from_unsafe(w_data);
    this->m_offset = zx_add_unsafe_u64(n_write, this_offset);
  }

  inline void push_cstring(ZXCString cstr, bool* overflow) {
    this->push_str(cstr.slice(), overflow);
  }

  inline void push_char(zx_u8 ch, bool* overflow) {
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
        __builtin_trap();
#endif
      }
    }
  }

  inline void push_u64(zx_u64 val, bool* overflow) {
    constexpr zx_u64 MAX_NUM_DIGITS = 20;
    zx_u8 digits[MAX_NUM_DIGITS];

    zx_u64 pos = MAX_NUM_DIGITS;
    do {
      pos--;
      digits[pos] = (zx_u8)'0' + (zx_u8)(val % 10);
      val /= 10;
    } while (val > 0);
    zx_u64 const len = zx_sub_unsafe_u64(MAX_NUM_DIGITS, pos);
    ZXSlice<zx_u8> const str = ZXSlice<zx_u8>::init_unsafe(&digits[pos], len);
    this->push_str(str, overflow);
  }

  inline void push_i64(zx_i64 val, bool* overflow) {
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
          __builtin_trap();
#endif
        }
      }
    }
  }
};

#endif

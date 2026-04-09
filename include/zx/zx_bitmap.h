#ifndef ZX_BITMAP_H
#define ZX_BITMAP_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

struct ZXBitmapIter {
  ZXSlice<zx_u64> m_storage;
  zx_u64 m_word;
  zx_u32 m_position;
  zx_u32 m_n_bits;

  inline bool next(zx_u32 *idx) {
    while (this->m_position < this->m_n_bits) {
      zx_u64 const word = this->m_word;
      if (word != 0) {
        zx_u8 const bit_pos = zx_ctz_u64(word);
        zx_u64 const mask = (zx_u64){1} << bit_pos;
        this->m_word &= ~mask;
        *idx = bit_pos + this->m_position;
        return true;
      } else {
        constexpr zx_u32 WORD_SIZE = 64;
        this->m_position += WORD_SIZE;
        this->m_word = *this->m_storage.index(this->m_position / WORD_SIZE);
      }
    }

    return false;
  }
};

struct ZXBitmap {
  ZXSliceMut<zx_u64> m_storage;
  zx_u32 m_n_bits;

  static inline ZXStatus init(ZXAlloc *alloc, zx_u32 n_bits, ZXBitmap *bitmap,
                              ZXErrStack *err_stack) {
    zx_u64 const bitmap_len = zx_chk_add_u64(n_bits, 63) / 64;
    ZXSliceMut<zx_u64> storage;
    ZX_ERR_TRY((alloc->alloc<zx_u64>(bitmap_len, &storage, err_stack)),
               err_stack);
    storage.memset(0);

    *bitmap = ZXBitmap{
        .m_storage = storage,
        .m_n_bits = n_bits,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 n_bits() const {
    return this->m_n_bits;
  }

  inline void reset() const {
    zx_u32 const bitmap_len = zx_chk_add_u32(this->m_n_bits, 63) / 64;
    zx_u32 const byte_size = zx_chk_mul_u32(bitmap_len, sizeof(zx_u64));
    __builtin_memset(this->m_storage, 0, byte_size);
  }

  inline bool get(zx_u32 idx) const {
    ZX_DBG_ASSERT(idx < this->m_n_bits);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 const byte = ((zx_u8 *)this->m_storage)[byte_index];
    return (byte & mask) != 0;
  }

  inline bool set(zx_u32 idx) {
    ZX_DBG_ASSERT(idx < this->m_n_bits);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 *byte_p = &((zx_u8 *)this->m_storage)[byte_index];
    zx_u8 const byte = *byte_p;
    bool const ret = (byte & mask) != 0;
    *byte_p = byte | mask;
    return ret;
  }

  inline bool unset(zx_u32 idx) {
    ZX_DBG_ASSERT(idx < this->m_n_bits);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 *byte_p = &((zx_u8 *)this->m_storage)[byte_index];
    zx_u8 const byte = *byte_p;
    bool const ret = (byte & mask) != 0;
    *byte_p = byte & ~mask;
    return ret;
  }

  inline ZXBitmapIter iter() const {
    return ZXBitmapIter{
        .m_storage = this->m_storage,
        .m_word = (this->m_n_bits > 0) ? this->m_storage[0] : 0,
        .m_position = 0,
        .m_n_bits = this->m_n_bits,
    };
  }
};

#endif

#ifndef ZX_BITMAP_H
#define ZX_BITMAP_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>

struct ZXBitmapIter {
  zx_u64 *storage;
  zx_u64 word;
  zx_u32 position;
  zx_u32 n_bits_;

  inline bool next(zx_u32 *idx) {
    while (this->position < this->n_bits_) {
      zx_u64 const word_ = this->word;
      if (word_ != 0) {
        zx_u8 const bit_pos = zx_ctz_u64(word_);
        zx_u64 const mask = (zx_u64){1} << bit_pos;
        this->word &= ~mask;
        *idx = bit_pos + this->position;
        return true;
      } else {
        constexpr zx_u32 WORD_SIZE = 64;
        this->position += WORD_SIZE;
        this->word = this->storage[this->position / WORD_SIZE];
      }
    }

    return false;
  }
};

struct ZXBitmap {
  zx_u64 *storage;
  zx_u32 n_bits_;

  static inline ZXStatus init(ZXAlloc *alloc, zx_u32 n_bits, ZXBitmap *bitmap,
                              ZXErrStack *err_stack) {
    zx_u32 const bitmap_len = zx_chk_add_u32(n_bits, 63) / 64;
    zx_u64 *storage = nullptr;
    ZX_ERR_TRY((alloc->alloc<zx_u64>(bitmap_len, &storage, err_stack)),
               err_stack);
    __builtin_memset(storage, 0, zx_chk_mul_u64(bitmap_len, sizeof(zx_u64)));

    *bitmap = ZXBitmap{
        .storage = storage,
        .n_bits_ = n_bits,
    };

    return ZXStatus::Ok;
  }

  __attribute__((always_inline)) inline zx_u32 n_bits() const {
    return this->n_bits_;
  }

  inline void reset() const {
    zx_u32 const bitmap_len = zx_chk_add_u32(this->n_bits_, 63) / 64;
    zx_u32 const byte_size = zx_chk_mul_u32(bitmap_len, sizeof(zx_u64));
    __builtin_memset(this->storage, 0, byte_size);
  }

  inline bool get(zx_u32 idx) const {
    ZX_DBG_ASSERT(idx < this->n_bits_);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 const byte = ((zx_u8 *)this->storage)[byte_index];
    return (byte & mask) != 0;
  }

  inline bool set(zx_u32 idx) {
    ZX_DBG_ASSERT(idx < this->n_bits_);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 *byte_p = &((zx_u8 *)this->storage)[byte_index];
    zx_u8 const byte = *byte_p;
    bool const ret = (byte & mask) != 0;
    *byte_p = byte | mask;
    return ret;
  }

  inline bool unset(zx_u32 idx) {
    ZX_DBG_ASSERT(idx < this->n_bits_);

    zx_u32 const byte_index = idx / 8;
    zx_u32 const bit_shift = idx % 8;
    zx_u8 const mask = (zx_u8){1} << bit_shift;
    zx_u8 *byte_p = &((zx_u8 *)this->storage)[byte_index];
    zx_u8 const byte = *byte_p;
    bool const ret = (byte & mask) != 0;
    *byte_p = byte & ~mask;
    return ret;
  }

  inline ZXBitmapIter iter() const {
    return ZXBitmapIter{
        .storage = this->storage,
        .word = (this->n_bits_ > 0) ? this->storage[0] : 0,
        .position = 0,
        .n_bits_ = this->n_bits_,
    };
  }
};

#endif

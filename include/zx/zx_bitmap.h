#ifndef ZX_BITMAP_H
#define ZX_BITMAP_H

#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_slice.h>

enum class [[nodiscard]] ZXBitmapInitStatus : zx_u8_c {
  Ok = 0,
  OutOfMemory,
  AllocFail,
  IntegerOverflow,
  Nullptr,
};

enum class [[nodiscard]] ZXBitmapAccessStatus : zx_u8_c {
  Set = 0,
  Unset,
  OutOfBounds,
};

enum class [[nodiscard]] ZXBitmapIterStatus : zx_u8_c {
  Continue = 0,
  Finished,
  Nullptr,
};

struct ZXBitmapIter {
  ZXSlice<zx_u64> m_storage;
  zx_u64 m_n_bits;
  zx_u64 m_position;
  zx_u64 m_word;

  inline ZXBitmapIterStatus next(zx_u64* idx) {
    if (idx == nullptr) {
      return ZXBitmapIterStatus::Nullptr;
    }

    while (true) {
      zx_u64 const word = this->m_word;
      if (word != 0) {
        zx_u8 const bit_pos = zx_ctz_u64(word);
        zx_u64 const mask = zx_shl_unsafe_u64(1, bit_pos);
        this->m_word &= ~mask;
        *idx = zx_add_unsafe_u64(bit_pos, this->m_position);
        return ZXBitmapIterStatus::Continue;
      } else {
        constexpr zx_u64 WORD_SIZE = 64;
        zx_u64 const new_pos = zx_add_unsafe_u64(this->m_position, WORD_SIZE);
        if (new_pos >= this->m_n_bits) {
          return ZXBitmapIterStatus::Finished;
        }
        this->m_word = this->m_storage.read_unsafe(new_pos / WORD_SIZE);
        this->m_position = new_pos;
      }
    }
  }
};

struct ZXBitmap {
  ZXSliceMut<zx_u64> m_storage;
  zx_u64 m_n_bits;

  static inline ZXBitmapInitStatus init(ZXAlloc* alloc,
                                        zx_u64 n_bits,
                                        ZXBitmap* bitmap) {
    if (alloc == nullptr || bitmap == nullptr) {
      return ZXBitmapInitStatus::IntegerOverflow;
    }

    bool overflow = false;
    zx_u64 const bitmap_len =
        zx_div_u64(zx_add_u64(n_bits, 63, &overflow), 64, &overflow);

    if (overflow) {
      return ZXBitmapInitStatus::IntegerOverflow;
    }

    ZXSliceMut<zx_u64> storage;
    switch (alloc->alloc<zx_u64>(bitmap_len, &storage)) {
      case ZXAllocStatus::Ok: {
        break;
      }
      case ZXAllocStatus::OutOfMemory: {
        return ZXBitmapInitStatus::OutOfMemory;
        break;
      }
      default: {
        return ZXBitmapInitStatus::AllocFail;
        break;
      }
    }
    storage.memset(0);

    *bitmap = ZXBitmap{
        .m_storage = storage,
        .m_n_bits = n_bits,
    };
    return ZXBitmapInitStatus::Ok;
  }

  inline zx_u64 n_bits() const { return this->m_n_bits; }

  inline void reset() const { this->m_storage.memset(0); }

  inline ZXBitmapAccessStatus get(zx_u64 idx) const {
    if (idx >= this->m_n_bits) {
      return ZXBitmapAccessStatus::OutOfBounds;
    }

    zx_u64 const byte_index = zx_div_unsafe_u64(idx, 8);
    zx_u8 const bit_shift =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(idx, 8));
    zx_u8 const mask = zx_shl_unsafe_u8(1, bit_shift);
    zx_u8 const byte =
        this->m_storage.cast_unsafe<zx_u8>().read_unsafe(byte_index);
    if ((byte & mask) != 0) {
      return ZXBitmapAccessStatus::Set;
    } else {
      return ZXBitmapAccessStatus::Unset;
    }
  }

  // returns the old state of the bit
  inline ZXBitmapAccessStatus set(zx_u64 idx) {
    if (idx >= this->m_n_bits) {
      return ZXBitmapAccessStatus::OutOfBounds;
    }

    zx_u64 const byte_index = zx_div_unsafe_u64(idx, 8);
    zx_u8 const bit_shift =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(idx, 8));
    zx_u8 const mask = zx_shl_unsafe_u8(1, bit_shift);
    zx_u8* byte_p =
        this->m_storage.cast_unsafe<zx_u8>().offset_unsafe(byte_index);
    zx_u8 const byte = *byte_p;
    *byte_p = byte | mask;
    if ((byte & mask) != 0) {
      return ZXBitmapAccessStatus::Set;
    } else {
      return ZXBitmapAccessStatus::Unset;
    }
  }

  // returns the old state of the bit
  inline ZXBitmapAccessStatus unset(zx_u64 idx) {
    if (idx >= this->m_n_bits) {
      return ZXBitmapAccessStatus::OutOfBounds;
    }

    zx_u64 const byte_index = zx_div_unsafe_u64(idx, 8);
    zx_u8 const bit_shift =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(idx, 8));
    zx_u8 const mask = zx_shl_unsafe_u8(1, bit_shift);
    zx_u8* byte_p =
        this->m_storage.cast_unsafe<zx_u8>().offset_unsafe(byte_index);
    zx_u8 const byte = *byte_p;
    *byte_p = byte & ~mask;
    if ((byte & mask) != 0) {
      return ZXBitmapAccessStatus::Set;
    } else {
      return ZXBitmapAccessStatus::Unset;
    }
  }

  inline ZXBitmapIter iter() const {
    return ZXBitmapIter{
        .m_storage = this->m_storage.as_const(),
        .m_word = (this->m_n_bits > 0) ? this->m_storage.read_unsafe(0) : 0,
        .m_position = 0,
        .m_n_bits = this->m_n_bits,
    };
  }
};

#endif

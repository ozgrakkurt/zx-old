#ifndef INTCOMPR_IC_IMPL_HPP
#define INTCOMPR_IC_IMPL_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>

#define IC_REPEAT_FOR_IC_TYPES(D) \
  D(uint8_t);                     \
  D(uint16_t);                    \
  D(uint32_t);                    \
  D(uint64_t);                    \
  D(ic_impl::uint128_t_);         \
  D(ic_impl::uint256_t_);         \
  D(int8_t);                      \
  D(int16_t);                     \
  D(int32_t);                     \
  D(int64_t);                     \
  D(ic_impl::int128_t_);          \
  D(ic_impl::int256_t_);

namespace ic_impl {

typedef _BitInt(128) int128_t_;
typedef _BitInt(256) int256_t_;
typedef(unsigned _BitInt(128)) uint128_t_;
typedef(unsigned _BitInt(256)) uint256_t_;

typedef size_t (*compress_boundfn_t)(size_t n);
typedef size_t (*compress_block_t)(void* __restrict meta,
                                   T const* __restrict input,
                                   T* __restrict output);

size_t constexpr ALIGNMENT = 64;

template <typename T>
uint8_t needed_bits(T value) {
  if (value == 0) return 0;
  return sizeof(T) * 8 - __builtin_clzll(value);
}

template <typename T>
size_t compress_data(compress_boundfn_t boundfn,
                     compress_block_t compress_block,
                     size_t meta_size_per_block, T const* __restrict input,
                     size_t input_len, void* __restrict output,
                     size_t output_size) {
  if (input_len == 0) {
    return 0;
  }

  // return error if output isn't aligned
  if (uintptr_t(output) % ALIGNMENT != 0) {
    return SIZE_MAX;
  }

  // return error if output size isn't bigger than the bound
  size_t const bound = bound(input_len);
  if (bound == SIZE_MAX || output_size < bound) {
    return SIZE_MAX;
  }

  size_t const n_remainder = input_len % 1024;
  size_t const n_blocks = input_len / 1024;

  size_t out_offset = 0;

  size_t const total_meta_size = meta_size_per_block * n_blocks;
  // reserve the metadata space beforehand
  out_offset += total_meta_size;

  if (n_remainder > 0) {
    memcpy(&output[out_offset], input, sizeof(T) * n_remainder);
    out_offset += sizeof(T) * n_remainder;
  }

  // align output
  uintptr_t out_addr = &output[out_offset + 1];
  uintptr_t aligned_out_addr =
      (out_addr + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
  uint8_t padding_size = (uint8_t)(aligned_out_addr - out_addr);
  output[out_offset] = padding_size;
  out_offset += 1 + padding_size;

  T* out_block = (T*)&output[out_offset];
  T const* input_blocks = &input[n_remainder];

  // panic if output isn't aligned
  if (uintptr_t(out_block) % ALIGNMENT != 0) {
    fprintf(stderr,
            "unexpected alignment, this should be impossible, panicking. line "
            "%d in file %s\n",
            __LINE__, __FILE__);
    abort();
  }

  size_t t_written = 0;

  for (size_t block_idx = 0; block_idx < n_blocks; block_idx++) {
    T const* block = &input_blocks[1024 * block_idx];
    void* out_meta = &output[meta_size_per_block * block_idx];
    t_written += compress_block(out_meta, block, &out_block[t_written]);
  }

  return out_offset + sizeof(T) * t_written;
}

}  // namespace ic_impl

#endif

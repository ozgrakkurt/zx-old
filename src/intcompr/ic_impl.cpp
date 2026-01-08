#include "./ic_impl.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace intcomp {

using namespace ic_impl;

inline bool is_err(size_t res) { return res == SIZE_MAX; }

template <typename T>
size_t compress_data(compress_boundfn_t boundfn,
                     compress_block_t compress_block,
                     size_t meta_size_per_block, void const* __restrict input,
                     size_t input_len, uint8_t* __restrict output,
                     size_t output_size) {
  if (input_len == 0) {
    return 0;
  }

  // return error if output isn't aligned
  if (size_t(output) % ALIGNMENT != 0) {
    return SIZE_MAX;
  }

  // return error if output size isn't bigger than the bound
  size_t const bound = boundfn(input_len);
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
  size_t out_addr = size_t(&output[out_offset + 1]);
  size_t aligned_out_addr = (out_addr + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
  uint8_t padding_size = (uint8_t)(aligned_out_addr - out_addr);
  output[out_offset] = padding_size;
  out_offset += 1 + padding_size;

  T* out_block = (T*)&output[out_offset];
  T const* input_blocks = &((T const*)input)[n_remainder];

  // panic if output isn't aligned
  if (size_t(out_block) % ALIGNMENT != 0) {
    fprintf(stderr,
            "unexpected alignment, this should be impossible, panicking. line "
            "%d in file %s\n",
            __LINE__, __FILE__);
    abort();
  }

  size_t t_written = 0;

  for (size_t block_idx = 0; block_idx < n_blocks; block_idx++) {
    T const* block = &input_blocks[1024 * block_idx];
    uint8_t* out_meta = &output[meta_size_per_block * block_idx];
    t_written += compress_block(out_meta, block, &out_block[t_written]);
  }

  return out_offset + sizeof(T) * t_written;
}

#define COMPRESS_DATA(T)                                           \
  template size_t compress_data<T>(                                \
      compress_boundfn_t boundfn, compress_block_t compress_block, \
      size_t meta_size_per_block, void const* __restrict input,    \
      size_t input_len, uint8_t* __restrict output, size_t output_size);
IC_REPEAT_FOR_IC_TYPES_UNSIGNED(COMPRESS_DATA);

}  // namespace intcomp

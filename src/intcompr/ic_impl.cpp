#include "./ic_impl.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace ic_impl {

typedef union u128_cast {
  uint128_t_ v;
  uint64_t arr[2];
} u128_cast;

typedef union u256_cast {
  uint256_t_ v;
  uint64_t arr[4];
} u256_cast;

template <int I>
void take128(uint128_t_ const *__restrict input, uint64_t *__restrict output) {
  for (size_t i=0; i<1024; i++) {
    u128_cast caster;
    caster.v = input[i];
    output[i] = uint64_t(caster.arr[I]);
  }
}

template <int I>
void take256(uint256_t_ const *__restrict input, uint64_t *__restrict output) {
  for (size_t i=0; i<1024; i++) {
    u256_cast caster;
    caster.v = input[i];
    output[i] = uint64_t(caster.arr[I]);
  }
}

void combine128(
  uint64_t const *__restrict input_0,
  uint64_t const *__restrict input_1,
  uint128_t_ *__restrict output
) {
  for (size_t i=0; i<1024; i++) {
      u128_cast caster;
      caster.arr[0] = input_0[i];
      caster.arr[1] = input_1[i];
      output[i] = caster.v;
  }
}

void combine256(
  uint64_t const *__restrict input_0,
  uint64_t const *__restrict input_1,
  uint64_t const *__restrict input_2,
  uint64_t const *__restrict input_3,
  uint256_t_ *__restrict output
) {
  for (size_t i=0; i<1024; i++) {
    u256_cast caster;
    caster.arr[0] = input_0[i];
    caster.arr[1] = input_1[i];
    caster.arr[2] = input_2[i];
    caster.arr[3] = input_3[i];
    output[i] = caster.v;
  }
}

template <typename T>
size_t kernel(compress_block_t compress_block, uint8_t *__restrict meta, void const* __restrict input, uint8_t* __restrict output);

template <typename T>
size_t kernel_simple(compress_block_t compress_block, uint8_t *__restrict meta, void const* __restrict input, uint8_t* __restrict output) {
  return compress_block(meta, input, output);
}

template <typename T, typename U>
size_t kernel_signed(compress_block_t compress_block, uint8_t *__restrict meta, void const* __restrict input, uint8_t* __restrict output) {

}

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
IC_REPEAT_FOR_IC_TYPES(COMPRESS_DATA);

}  // namespace ic_impl

namespace intcomp {
  bool is_err(size_t res) { return res == SIZE_MAX; }
}

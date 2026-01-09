#include "./ic_impl.hpp"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "./ic_zigzag.hpp"

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

typedef union u128_cast {
  uint128_t_ v;
  uint64_t arr[2];
} u128_cast;

typedef union u256_cast {
  uint256_t_ v;
  uint64_t arr[4];
} u256_cast;

template <int I>
void take128(uint128_t_ const* __restrict input, uint64_t* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    u128_cast caster;
    caster.v = input[i];
    output[i] = uint64_t(caster.arr[I]);
  }
}

template <int I>
void take256(uint256_t_ const* __restrict input, uint64_t* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    u256_cast caster;
    caster.v = input[i];
    output[i] = uint64_t(caster.arr[I]);
  }
}

void combine128(uint64_t const* __restrict input_0,
                uint64_t const* __restrict input_1,
                uint128_t_* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    u128_cast caster;
    caster.arr[0] = input_0[i];
    caster.arr[1] = input_1[i];
    output[i] = caster.v;
  }
}

void combine256(uint64_t const* __restrict input_0,
                uint64_t const* __restrict input_1,
                uint64_t const* __restrict input_2,
                uint64_t const* __restrict input_3,
                uint256_t_* __restrict output) {
  for (size_t i = 0; i < 1024; i++) {
    u256_cast caster;
    caster.arr[0] = input_0[i];
    caster.arr[1] = input_1[i];
    caster.arr[2] = input_2[i];
    caster.arr[3] = input_3[i];
    output[i] = caster.v;
  }
}

template <typename T>
size_t decompress_kernel(decompress_block_t decompress_block,
                         size_t meta_size_per_block,
                         uint8_t const* __restrict meta,
                         uint8_t const* __restrict input, T* __restrict output);

template <typename T>
size_t decompress_kernel_simple(decompress_block_t decompress_block,
                                size_t meta_size_per_block,
                                uint8_t const* __restrict meta,
                                uint8_t const* __restrict input,
                                T* __restrict output) {
  (void)meta_size_per_block;
  return decompress_block(meta, input, output);
}

template <typename T, typename U>
size_t decompress_kernel_signed(decompress_block_t decompress_block,
                                size_t meta_size_per_block,
                                uint8_t const* __restrict meta,
                                uint8_t const* __restrict input,
                                T* __restrict output) {
  alignas(64) U zigzagged[1024];
  size_t const n_read = decompress_kernel<U>(
      decompress_block, meta_size_per_block, meta, input, zigzagged);

  if (n_read == SIZE_MAX) {
    return SIZE_MAX;
  }

  ic_zigzag::zigzag_decode<T, U>(zigzagged, (U*)output);

  return n_read;
}

template <typename T, typename I>
size_t decompress_kernel_128(decompress_block_t decompress_block,
                             size_t meta_size_per_block,
                             uint8_t const* __restrict meta,
                             uint8_t const* __restrict input,
                             T* __restrict output) {
  auto in = (I const*)input;

  alignas(64) I out[1024];
}

template <typename T>
size_t compress_kernel(compress_block_t compress_block,
                       size_t meta_size_per_block, uint8_t* __restrict meta,
                       T const* __restrict input, uint8_t* __restrict output);

template <typename T>
size_t compress_kernel_simple(compress_block_t compress_block,
                              size_t meta_size_per_block,
                              uint8_t* __restrict meta,
                              T const* __restrict input,
                              uint8_t* __restrict output) {
  (void)meta_size_per_block;
  return compress_block(meta, input, output);
}

template <typename T, typename U>
size_t compress_kernel_signed(compress_block_t compress_block,
                              size_t meta_size_per_block,
                              uint8_t* __restrict meta,
                              T const* __restrict input,
                              uint8_t* __restrict output) {
  alignas(64) U zigzagged[1024];
  ic_zigzag::zigzag_encode<T, U>(input, zigzagged);

  return compress_kernel<U>(compress_block, meta_size_per_block, meta,
                            zigzagged, output);
}

template <typename T, typename I>
size_t compress_kernel_128(compress_block_t compress_block,
                           size_t meta_size_per_block, uint8_t* __restrict meta,
                           T const* __restrict input,
                           uint8_t* __restrict output) {
  alignas(64) I in[1024];

  take128<0>((uint128_t_ const*)input, (uint64_t*)in);
  size_t const b0 =
      compress_kernel<I>(compress_block, meta_size_per_block, meta, in, output);

  take128<1>((uint128_t_ const*)input, (uint64_t*)in);
  size_t const b1 =
      compress_kernel<I>(compress_block, meta_size_per_block,
                         &meta[meta_size_per_block], in, &output[b0]);

  return b0 + b1;
}

template <typename T, typename I>
size_t compress_kernel_256(compress_block_t compress_block,
                           size_t meta_size_per_block, uint8_t* __restrict meta,
                           T const* __restrict input,
                           uint8_t* __restrict output) {
  alignas(64) I in[1024];

  take256<0>((uint256_t_ const*)input, (uint64_t*)in);
  size_t const b0 =
      compress_kernel<I>(compress_block, meta_size_per_block, meta, in, output);

  take256<1>((uint256_t_ const*)input, (uint64_t*)in);
  size_t const b1 =
      compress_kernel<I>(compress_block, meta_size_per_block,
                         &meta[meta_size_per_block], in, &output[b0]);

  take256<2>((uint256_t_ const*)input, (uint64_t*)in);
  size_t const b2 =
      compress_kernel<I>(compress_block, meta_size_per_block,
                         &meta[2 * meta_size_per_block], in, &output[b0 + b1]);

  take256<3>((uint256_t_ const*)input, (uint64_t*)in);
  size_t const b3 = compress_kernel<I>(compress_block, meta_size_per_block,
                                       &meta[3 * meta_size_per_block], in,
                                       &output[b0 + b1 + b2]);

  return b0 + b1 + b2 + b3;
}

template <>
size_t compress_kernel<uint16_t>(compress_block_t compress_block,
                                 size_t meta_size_per_block,
                                 uint8_t* __restrict meta,
                                 uint16_t const* __restrict input,
                                 uint8_t* __restrict output) {
  return compress_kernel_simple<uint16_t>(compress_block, meta_size_per_block,
                                          meta, input, output);
}
template <>
size_t compress_kernel<uint32_t>(compress_block_t compress_block,
                                 size_t meta_size_per_block,
                                 uint8_t* __restrict meta,
                                 uint32_t const* __restrict input,
                                 uint8_t* __restrict output) {
  return compress_kernel_simple<uint32_t>(compress_block, meta_size_per_block,
                                          meta, input, output);
}
template <>
size_t compress_kernel<uint64_t>(compress_block_t compress_block,
                                 size_t meta_size_per_block,
                                 uint8_t* __restrict meta,
                                 uint64_t const* __restrict input,
                                 uint8_t* __restrict output) {
  return compress_kernel_simple<uint64_t>(compress_block, meta_size_per_block,
                                          meta, input, output);
}
template <>
size_t compress_kernel<uint128_t_>(compress_block_t compress_block,
                                   size_t meta_size_per_block,
                                   uint8_t* __restrict meta,
                                   uint128_t_ const* __restrict input,
                                   uint8_t* __restrict output) {
  return compress_kernel_128<uint128_t_, uint64_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<uint256_t_>(compress_block_t compress_block,
                                   size_t meta_size_per_block,
                                   uint8_t* __restrict meta,
                                   uint256_t_ const* __restrict input,
                                   uint8_t* __restrict output) {
  return compress_kernel_256<uint256_t_, uint64_t>(
      compress_block, meta_size_per_block, meta, input, output);
}

template <>
size_t compress_kernel<int8_t>(compress_block_t compress_block,
                               size_t meta_size_per_block,
                               uint8_t* __restrict meta,
                               int8_t const* __restrict input,
                               uint8_t* __restrict output) {
  return compress_kernel_signed<int8_t, uint8_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<int16_t>(compress_block_t compress_block,
                                size_t meta_size_per_block,
                                uint8_t* __restrict meta,
                                int16_t const* __restrict input,
                                uint8_t* __restrict output) {
  return compress_kernel_signed<int16_t, uint16_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<int32_t>(compress_block_t compress_block,
                                size_t meta_size_per_block,
                                uint8_t* __restrict meta,
                                int32_t const* __restrict input,
                                uint8_t* __restrict output) {
  return compress_kernel_signed<int32_t, uint32_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<int64_t>(compress_block_t compress_block,
                                size_t meta_size_per_block,
                                uint8_t* __restrict meta,
                                int64_t const* __restrict input,
                                uint8_t* __restrict output) {
  return compress_kernel_signed<int64_t, uint64_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<int128_t_>(compress_block_t compress_block,
                                  size_t meta_size_per_block,
                                  uint8_t* __restrict meta,
                                  int128_t_ const* __restrict input,
                                  uint8_t* __restrict output) {
  return compress_kernel_128<int128_t_, int64_t>(
      compress_block, meta_size_per_block, meta, input, output);
}
template <>
size_t compress_kernel<int256_t_>(compress_block_t compress_block,
                                  size_t meta_size_per_block,
                                  uint8_t* __restrict meta,
                                  int256_t_ const* __restrict input,
                                  uint8_t* __restrict output) {
  return compress_kernel_256<int256_t_, int64_t>(
      compress_block, meta_size_per_block, meta, input, output);
}

template <typename T>
size_t compress_impl(compress_boundfn_t boundfn,
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

  size_t const total_meta_size =
      meta_size_per_block * n_blocks * n_block_metas_multiplier<T>();
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

  T const* input_blocks = &((T const*)input)[n_remainder];

  // panic if output isn't aligned
  if (size_t(&output[out_offset]) % ALIGNMENT != 0) {
    fprintf(stderr,
            "unexpected alignment, this should be impossible, panicking. line "
            "%d in file %s\n",
            __LINE__, __FILE__);
    abort();
  }

  for (size_t block_idx = 0; block_idx < n_blocks; block_idx++) {
    T const* block = &input_blocks[1024 * block_idx];
    uint8_t* out_meta = &output[meta_size_per_block * block_idx *
                                n_block_metas_multiplier<T>()];
    out_offset += compress_kernel<T>(compress_block, meta_size_per_block,
                                     out_meta, block, &output[out_offset]);

    assert(size_t(&output[out_offset]) % ALIGNMENT == 0);
  }

  return out_offset;
}

#define COMPRESS_DATA(T)                                           \
  template size_t compress_impl<T>(                                \
      compress_boundfn_t boundfn, compress_block_t compress_block, \
      size_t meta_size_per_block, void const* __restrict input,    \
      size_t input_len, uint8_t* __restrict output, size_t output_size);
IC_REPEAT_FOR_IC_TYPES(COMPRESS_DATA);

}  // namespace ic_impl

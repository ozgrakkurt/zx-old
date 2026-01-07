#include <climits>
#include <cstddef>
#include <cstdlib>

#include "./ic_impl.hpp"

namespace intcomp {

template <typename T>
size_t bit_pack_compress_bound(size_t n) {
  // return error if n is too big
  if (n >= INT_MAX) {
    return SIZE_MAX;
  }

  size_t const n_blocks = n / 1024;
  size_t const n_remainder = n % 1024;

  size_t constexpr size_per_block = sizeof(uint8_t) + 1024 * sizeof(T);

  size_t constexpr padding_size = sizeof(uint8_t);

  return padding_size + size_per_block * n_blocks + n_remainder * sizeof(T);
}

template <typename T>
size_t 

}  // namespace intcomp

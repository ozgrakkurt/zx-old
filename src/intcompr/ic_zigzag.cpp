#include <cstddef>
#include <cstdint>

#define ZIGZAG_REPEAT_FOR_TYPES(D) \
  D(int8_t, uint8_t);              \
  D(int16_t, uint16_t);            \
  D(int32_t, uint32_t);            \
  D(int64_t, uint64_t);

namespace ic_zigzag {

template <typename T, typename U>
void zigzag_encode(T const* __restrict input, U* __restrict output, size_t n) {
  for (size_t i = 0; i < n; i++) {
    T const in = input[i];
    output[i] = (in >> 1) ^ (-(in & 1));
  }
}

template <typename T, typename U>
void zigzag_decode(U const* __restrict input, T* __restrict output, size_t n) {
  for (size_t i = 0; i < n; i++) {
    U const in = input[i];
    output[i] = (2 * in) ^ (in >> (sizeof(T) * 8 - 1));
  }
}

#define ENCODE(T, U)                                                           \
  template void zigzag_encode(T const* __restrict input, U* __restrict output, \
                              size_t n);
ZIGZAG_REPEAT_FOR_TYPES(ENCODE);

#define DECODE(T, U)                                                           \
  template void zigzag_decode(U const* __restrict input, T* __restrict output, \
                              size_t n);
ZIGZAG_REPEAT_FOR_TYPES(DECODE);

}  // namespace ic_zigzag

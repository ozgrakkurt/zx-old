#include <stdio.h>

#include <cstdint>
#include <cstdlib>
#include <fastlanes.hpp>

template <typename T>
T max(T const* values, size_t n) {
  if (n == 0) {
    fprintf(stderr, "can't find max if n==0\n");
    abort();
  }

  T m = values[0];

  for (size_t i = 1; i < n; i++) {
    T const v = values[i];
    m = v > m ? v : m;
  }

  return m;
}

template <typename T>
uint8_t needed_bits(T value) {
  if (value == 0) return 0;
  return sizeof(T) * 8 - __builtin_clzll(value);
}

template <typename T>
void run() {
  T input[1024];

  for (size_t i = 0; i < 1024; i++) {
    input[i] = (T)(i * 80);
  }

  T transposed[1024];

  fastlanes::transpose(input, transposed);

  T deltas[1024];

  fastlanes::delta_encode(transposed, transposed, deltas);

  T const m = max<T>(deltas, 1024);

  uint8_t const width = needed_bits<T>(m);

  T packed[1024];

  fastlanes::bit_pack(deltas, packed, width);

  T undelta[1024];

  fastlanes::bit_unpack_delta_decode(transposed, packed, undelta, width);

  T output[1024];

  fastlanes::untranspose(undelta, output);

  for (size_t i = 0; i < 1024; i++) {
    if (input[i] != output[i]) {
      abort();
    }
  }

  for (size_t i = 0; i < 1024; i++) {
    printf("%lu\n", output[i]);
  }
}

int main() {
  run<uint64_t>();

  return 0;
}

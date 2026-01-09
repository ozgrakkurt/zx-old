#ifndef IC_ZIGZAG_HPP
#define IC_ZIGZAG_HPP

#include <cstddef>

namespace ic_zigzag {

template <typename T, typename U>
void zigzag_encode(T const* __restrict input, U* __restrict output);

template <typename T, typename U>
void zigzag_decode(U const* __restrict input, T* __restrict output);

}  // namespace ic_zigzag

#endif

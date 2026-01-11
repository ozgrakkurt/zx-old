#ifndef BLIB_INT_HPP
#define BLIB_INT_HPP

#include <cstdint>

namespace blib {

typedef _BitInt(128) int128_t_;
typedef _BitInt(256) int256_t_;
typedef unsigned _BitInt(128) uint128_t_;
typedef unsigned _BitInt(256) uint256_t_;

}

#endif

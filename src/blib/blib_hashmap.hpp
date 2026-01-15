#ifndef BLIB_HASHMAP_HPP
#define BLIB_HASHMAP_HPP

namespace blib {

namespace internal {}

struct Metadata {
  uint8_t v;
};

template <typename K, typename V, bool eqfn(K left, K right)>
struct HashMap {
  void* buf;
  uint32_t const capacity;
  uint32_t size;
};

}  // namespace blib

#endif

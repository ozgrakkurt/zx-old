#ifndef BLIB_HASHMAP_HPP
#define BLIB_HASHMAP_HPP

#include <blib_alloc.hpp>
#include <blib_err.hpp>
#include <blib_int.hpp>
#include <cstddef>
#include <cstdint>

namespace blib {

namespace hashmap_ {
template <typename K, typename V>
struct Item {
  V value;
  K key;
};
}  // namespace hashmap_

struct Metadata {
  uint8_t v;
};

static size_t constexpr ALIGNMENT = 64;

static uint32_t constexpr MAX_LOAD_PERCENTAGE = 80;

template <typename K, typename V, bool eqfn(K left, K right)>
struct HashMap {
 private:
  void* buf;
  uint32_t const capacity_log2;
  uint32_t size;

  uint8_t* get_metadata_ptr() const { return this.buf; }

  Item<K, V>* get_items_ptr() const {
    return this.buf + uint32_t(uint32_t(1) << this.capacity_log2);
  }

 public:
  Status init(Alloc* alloc, uint32_t capacity) {
    uint32_t const alloc_cap =
        uint32_t(uint64_t(capacity) * 100 / MAX_LOAD_PERCENTAGE);
    uint32_t alloc_cap_p2;
    BLIB_ERR_TRY((next_power_of_two(alloc_cap, &alloc_cap_p2)));

    void* buf = nullptr;

    BLIB_ERR_TRY((alloc.aligned_alloc<T>(ALIGNMENT, capacity, &buf)));
    return Status::Ok;
  }
};

}  // namespace blib

#endif

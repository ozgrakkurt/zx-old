#include "./intcompr.hpp"

#include <cstddef>

namespace intcompr {
bool is_decompress_error(size_t res) { return res == SIZE_MAX; }
}  // namespace intcompr

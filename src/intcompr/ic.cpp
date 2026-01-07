#include <cstddef>
#include <cstdlib>

#include "./ic_impl.hpp"

namespace intcomp {

inline bool is_err(size_t res) { return res == SIZE_MAX; }

}  // namespace intcomp

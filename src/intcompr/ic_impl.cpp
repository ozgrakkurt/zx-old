#include "./ic_impl.hpp"

#include <cstddef>
#include <cstdlib>

namespace intcomp {

inline bool is_err(size_t res) { return res == SIZE_MAX; }

}  // namespace intcomp

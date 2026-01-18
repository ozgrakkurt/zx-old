#ifndef BLIB_SLUB_HPP
#define BLIB_SLUB_HPP

#include <cstddef>
#include "./blib_alloc.hpp"

namespace blib {

namespace slub_ {
  
struct Page {
  Page* next;
};

}

struct Slub {
  Alloc inner;
  size_t page_size;
  Page* head; 
};

}

#endif

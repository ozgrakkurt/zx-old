#include <zx/zx_int.h>

static inline void assert(bool test) {
  if (!test) {
    __builtin_trap();
  }
}

int main(void) {
  bool overflow = false;
  zx_add_u8(ZX_U8_MAX, 1, &overflow);
  assert(!overflow);

  return 0;
}

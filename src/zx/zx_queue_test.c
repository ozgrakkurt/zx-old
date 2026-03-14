#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_queue.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 key_type;

static inline ZXStatus smoke_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXQueue queue;
  ZX_ERR_TRY((ZXQueue_init(alloc, CAP, alignof(key_type), sizeof(key_type),
                           &queue, err_stack)),
             err_stack);

  // should be empty
  key_type key = 69;
  ZX_ERR_ASSERT((!ZXQueue_pop(&queue, sizeof(key_type), &key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // push
    key = i;
    ZX_ERR_ASSERT((ZXQueue_push(&queue, &key, sizeof(key_type))), err_stack);
  }

  // should be full
  key = 69;
  ZX_ERR_ASSERT(!ZXQueue_push(&queue, &key, sizeof(key_type)), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // pop
    ZX_ERR_ASSERT((ZXQueue_pop(&queue, sizeof(key_type), &key)), err_stack);
    ZX_ERR_ASSERT((i == key), err_stack);
  }

  // should be empty
  key = 69;
  ZX_ERR_ASSERT((!ZXQueue_pop(&queue, sizeof(key_type), &key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  return ZXStatus_Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc *alloc, ZXErrStack *err_stack) {
  ZXQueue queue;
  ZX_ERR_TRY((ZXQueue_init(alloc, 0, alignof(key_type), sizeof(key_type),
                           &queue, err_stack)),
             err_stack);

  // should be empty
  key_type key = 69;
  ZX_ERR_ASSERT((!ZXQueue_pop(&queue, sizeof(key_type), &key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  // should be full
  key = 69;
  ZX_ERR_ASSERT(!ZXQueue_push(&queue, &key, sizeof(key_type)), err_stack);

  return ZXStatus_Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack_init();

  zx_u8 *mem = nullptr;
  ZX_ERR_TRY_FATAL((zx_page_alloc_4kb(MEMORY_SIZE, &mem, &err_stack)),
                   &err_stack);

  ZXAlloc alloc = ZXAlloc_init(mem, MEMORY_SIZE);

  ZX_ERR_TRY_FATAL((smoke_test(&alloc, &err_stack)), &err_stack);
  ZXAlloc_reset(&alloc);

  ZX_ERR_TRY_FATAL((zero_size_test(&alloc, &err_stack)), &err_stack);

  ZX_ERR_TRY_FATAL((zx_page_alloc_free(mem, MEMORY_SIZE, &err_stack)),
                   &err_stack);
  return 0;
}

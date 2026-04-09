#include <zx/zx_alloc.h>
#include <zx/zx_err.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_queue.h>

constexpr zx_u64 MEMORY_SIZE = 1 << 20;
constexpr zx_u64 CAP = 3169;
typedef zx_u256 key_type;

using Queue = ZXQueue<key_type>;

static inline ZXStatus smoke_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  Queue queue;
  ZX_ERR_TRY((Queue::init(alloc, CAP, &queue, err_stack)), err_stack);

  // should be empty
  key_type key = 69;
  ZX_ERR_ASSERT((!queue.pop(&key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // push
    key = i;
    ZX_ERR_ASSERT((queue.push(&key)), err_stack);
  }

  // should be full
  key = 69;
  ZX_ERR_ASSERT(!queue.push(&key), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // pop
    ZX_ERR_ASSERT((queue.pop(&key)), err_stack);
    ZX_ERR_ASSERT((i == key), err_stack);
  }

  // should be empty
  key = 69;
  ZX_ERR_ASSERT((!queue.pop(&key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  return ZXStatus::Ok;
}

static inline ZXStatus smoke_test_in_place(ZXAlloc alloc,
                                           ZXErrStack *err_stack) {
  Queue queue;
  ZX_ERR_TRY((Queue::init(alloc, CAP, &queue, err_stack)), err_stack);

  // should be empty
  key_type key = 69;
  ZX_ERR_ASSERT((!queue.pop(&key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // push
    key = i;
    key_type *entry = queue.push_in_place();
    ZX_ERR_ASSERT((entry != nullptr), err_stack);
    *entry = key;
  }

  // should be full
  key_type *entry = queue.push_in_place();
  ZX_ERR_ASSERT((entry == nullptr), err_stack);

  for (zx_u64 i = 0; i < CAP; i++) {
    // pop
    ZX_ERR_ASSERT((queue.pop(&key)), err_stack);
    ZX_ERR_ASSERT((i == key), err_stack);
  }

  // should be empty
  key = 69;
  ZX_ERR_ASSERT((!queue.pop(&key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  return ZXStatus::Ok;
}

static inline ZXStatus zero_size_test(ZXAlloc alloc, ZXErrStack *err_stack) {
  Queue queue;
  ZX_ERR_TRY((Queue::init(alloc, 0, &queue, err_stack)), err_stack);

  // should be empty
  key_type key = 69;
  ZX_ERR_ASSERT((!queue.pop(&key)), err_stack);
  ZX_ERR_ASSERT((key == 69), err_stack);

  // should be full
  key = 69;
  ZX_ERR_ASSERT(!queue.push(&key), err_stack);

  return ZXStatus::Ok;
}

int main(void) {
  ZXErrStack err_stack = ZXErrStack::init();

  ZXDebugAlloc dbg_alloc;
  ZX_ERR_TRY_FATAL((ZXDebugAlloc::init(MEMORY_SIZE, &dbg_alloc, &err_stack)),
                   &err_stack);
  ZXAlloc alloc = dbg_alloc.make_alloc();

  ZX_ERR_TRY_FATAL((smoke_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((zero_size_test(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  ZX_ERR_TRY_FATAL((smoke_test_in_place(alloc, &err_stack)), &err_stack);
  dbg_alloc.reset();

  return 0;
}

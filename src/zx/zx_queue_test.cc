#include <zx/zx_alloc.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_queue.h>
#include <zx/zx_slice.h>

typedef zx_u256 key_type;

using Queue = ZXQueue<key_type>;

static void assert(bool cond) {
  if (!cond) {
    __builtin_trap();
  }
}

static void smoke_test_inner(Queue *q, zx_u32 cap) {
  assert(q->capacity_left() == cap);
  assert(q->len() == 0);

  key_type key = 69;
  assert(ZXQueuePopStatus::Empty == q->pop(&key));
  assert(69 == key);

  for (zx_u32 i = 0; i < cap; i++) {
    key = i;
    assert(ZXQueuePushStatus::Ok == q->push(&key));
    assert(q->len() == zx_add_unsafe_u32(1, i));
    assert(q->capacity_left() ==
           zx_sub_unsafe_u32(zx_sub_unsafe_u32(cap, i), 1));
  }

  assert(q->capacity_left() == 0);
  key = 69;
  assert(ZXQueuePushStatus::OutOfCapacity == q->push(&key));

  for (zx_u32 i = 0; i < cap; i++) {
    assert(ZXQueuePopStatus::Ok == q->pop(&key));
    assert(key == i);
    assert(q->len() == zx_sub_unsafe_u32(zx_sub_unsafe_u32(cap, i), 1));
    assert(q->capacity_left() == zx_add_unsafe_u32(1, i));
  }

  assert(q->capacity_left() == cap);
  assert(q->len() == 0);
  key = 69;
  assert(ZXQueuePopStatus::Empty == q->pop(&key));
  assert(69 == key);
}

static void smoke_test(ZXAlloc *alloc, zx_u32 cap) {
  Queue queue;
  assert(ZXQueueInitStatus::Ok == Queue::init(alloc, cap, &queue));

  smoke_test_inner(&queue, cap);
  queue.reset();
  smoke_test_inner(&queue, cap);
  queue.reset();
  smoke_test_inner(&queue, cap);
}

static inline void zero_size_test() {
  ZXAlloc noalloc = ZXAlloc::init(ZXSliceMut<zx_u8>::init_dangling());

  Queue queue;
  assert(ZXQueueInitStatus::OutOfMemory == Queue::init(&noalloc, 69, &queue));
  assert(ZXQueueInitStatus::OutOfMemory == Queue::init(&noalloc, 1, &queue));

  assert(ZXQueueInitStatus::Ok == Queue::init(&noalloc, 0, &queue));

  // should be empty
  key_type key = 69;
  assert(ZXQueuePopStatus::Empty == queue.pop(&key));

  // should be full
  key = 69;
  assert(ZXQueuePushStatus::OutOfCapacity == queue.push(&key));

  // should be empty
  key = 69;
  assert(ZXQueuePopStatus::Empty == queue.pop(&key));

  // should be full
  key = 69;
  assert(ZXQueuePushStatus::OutOfCapacity == queue.push(&key));
}

int main(void) {
  constexpr zx_u64 MEMORY_SIZE = 1 << 20;

  ZXSliceMut<zx_u8> mem;
  assert(zx_page_alloc_4kb(MEMORY_SIZE, &mem) == ZXPageAllocStatus::Ok);

  zero_size_test();

  ZXAlloc alloc = ZXAlloc::init(mem);

  constexpr zx_u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129,  3169};
  constexpr zx_u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u32 i = 0; i < n_lengths; i++) {
    zx_u32 const len = lengths[i];
    alloc.reset();
    smoke_test(&alloc, len);
  }

  assert(zx_page_free(mem) == ZXPageAllocStatus::Ok);
  return 0;
}

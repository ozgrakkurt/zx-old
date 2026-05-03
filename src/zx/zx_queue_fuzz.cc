#include <zx/zx_alloc.h>
#include <zx/zx_fuzz.h>
#include <zx/zx_int.h>
#include <zx/zx_page_alloc.h>
#include <zx/zx_queue.h>
#include <zx/zx_slice.h>

static void assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

struct Ctx {
  ZXAlloc alloc;
};

static void fuzz_one(void* usr_ctx, ZXFuzzInput* fuzz_input) {
  assert(usr_ctx != nullptr);
  ZXAlloc* alloc = &((Ctx*)usr_ctx)->alloc;
  alloc->reset();

  constexpr zx_u32 max_capacity = 128;
  zx_u32 const capacity = zx_cast_unsafe_to_u32<zx_u64>(
      zx_mod_unsafe_u64(fuzz_input->next(), max_capacity));

  constexpr zx_u32 max_n_ops = zx_mul_unsafe_u32(max_capacity, 4);
  zx_u32 const n_ops = zx_cast_unsafe_to_u32<zx_u64>(
      zx_mod_unsafe_u64(fuzz_input->next(), max_n_ops));

  ZXQueue<zx_u32> queue;
  assert(ZXQueueInitStatus::Ok ==
         ZXQueue<zx_u32>::init(alloc, capacity, &queue));

  zx_u32 start = 0;
  zx_u32 end = 0;

  for (zx_u32 i = 0; i < n_ops; i++) {
    zx_u8 const op =
        zx_cast_unsafe_to_u8<zx_u64>(zx_mod_unsafe_u64(fuzz_input->next(), 2));
    switch (op) {
      // push
      case 0: {
        ZXQueuePushStatus const status = queue.push(&end);
        if (status == ZXQueuePushStatus::Ok) {
          end = zx_add_unsafe_u32(end, 1);
        } else if (status != ZXQueuePushStatus::OutOfCapacity) {
          __builtin_trap();
        }
        break;
      }
      // pop
      case 1: {
        zx_u32 val = ZX_U32_MAX;
        ZXQueuePopStatus const status = queue.pop(&val);
        if (status == ZXQueuePopStatus::Ok) {
          assert(val == start);
          start = zx_add_unsafe_u32(start, 1);
        } else if (status != ZXQueuePopStatus::Empty) {
          __builtin_trap();
        }

        break;
      }
      default: {
        __builtin_trap();
      }
    }
  }

  zx_u32 val = ZX_U32_MAX;
  while (queue.pop(&val) == ZXQueuePopStatus::Ok) {
    assert(val == start);
    start = zx_add_unsafe_u32(start, 1);
  }

  assert(start == end);
}

int main(void) {
  constexpr zx_u64 mem_cap = zx_u64{1} << 30;
  ZXSliceMut<zx_u8> mem;
  assert(ZXPageAllocStatus::Ok == zx_page_alloc_4kb(mem_cap, &mem));

  ZXAlloc alloc = ZXAlloc::init(mem);

  constexpr zx_u64 fuzz_mem_cap = zx_u64{1} << 27;
  ZXSliceMut<zx_u8> fuzz_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(fuzz_mem_cap, &fuzz_mem));

  ZXAlloc fuzz_alloc = ZXAlloc::init(fuzz_mem);

  constexpr zx_u64 op_mem_cap = zx_sub_unsafe_u64(mem_cap, fuzz_mem_cap);
  ZXSliceMut<zx_u8> op_mem;
  assert(ZXAllocStatus::Ok == alloc.alloc<zx_u8>(op_mem_cap, &op_mem));

  ZXAlloc op_alloc = ZXAlloc::init(op_mem);
  Ctx ctx = Ctx{
      .alloc = op_alloc,
  };
  zx_fuzz(&fuzz_alloc, (void*)&ctx, fuzz_one);
  return 0;
}

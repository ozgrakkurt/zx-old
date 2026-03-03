#ifndef ZX_FUZZ_MUT_H
#define ZX_FUZZ_MUT_H

#include "./zx_fuzz_inputs.h"
#include "./zx_fuzz_mut_magic/zx_fuzz_mut_magic.h"
#include <zx/zx_def.h>
#include <zx/zx_int.h>
#include <zx/zx_xoshiro256.h>

typedef enum ZXFuzzMut : unsigned char {
  ZX_FUZZ_MUT_BYTE = 0,
  ZX_FUZZ_MUT_BIT = 1,
  ZX_FUZZ_MUT_INCREMENT = 2,
  ZX_FUZZ_MUT_DECREMENT = 3,
  ZX_FUZZ_MUT_NEGATE = 4,
  ZX_FUZZ_MUT_ADD = 5,
  ZX_FUZZ_MUT_SUB = 6,
  ZX_FUZZ_MUT_MEMSET = 7,
  ZX_FUZZ_MUT_MEM_CLR = 8,
  ZX_FUZZ_MUT_RESELECT = 9,
  ZX_FUZZ_MUT_MAGIC = 10,
  ZX_FUZZ_MUT_RANDOM_SET = 11,
} ZXFuzzMutOp;

constexpr zx_u8 ZX_FUZZ_MUT_N_KINDS = 12;

static inline ZXFuzzMut ZXFuzzMut_random(ZXXoshiro256 *rand) {
  zx_u64 const choice = ZXXoshiro256_next(rand);
  return (ZXFuzzMut)zx_rand_mod_u64(choice, ZX_FUZZ_MUT_N_KINDS);
}

static inline void ZXFuzzMut_apply(ZXFuzzMut op, )

typedef struct ZXFuzzMutator {
  ZXFuzzMut prev_mut;
  zx_u32 last_offset;
} ZXFuzzMutator;

static inline ZXFuzzMutator ZXFuzzMutator_init(void) {
  ZXFuzzMutator ret = {
    .prev_mut = ZX_FUZZ_MUT_BYTE,
    .last_offset = 0,
  };

  return ret;
}

// decrease the weight of last mutation and do a new one
static inline void ZXFuzzMutator_next(ZXFuzzMutator *mutator, ZXFuzzInputs *inputs,
                                        ZXXoshiro256 *rand, zx_u8 *fuzz_input,
                                        zx_u32 fuzz_input_len, bool repeat_last) {
  ZX_ASSERT((!repeat_last || !mutator->did_mut));

}

#endif

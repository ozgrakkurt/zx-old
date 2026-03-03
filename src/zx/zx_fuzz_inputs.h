#ifndef ZX_FUZZ_INPUTS_H
#define ZX_FUZZ_INPUTS_H

#include <zx/zx_alloc.h>
#include <zx/zx_assert.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_hashtable.h>
#include <zx/zx_int.h>
#include <zx/zx_xoshiro256.h>

// Used for saving fuzz input history
typedef struct ZXFuzzInputs {
  ZXHashtable signatures;
  zx_u32 n_inputs_cap;
  zx_u32 n_inputs;
  zx_u32 data_size_cap;
  zx_u32 data_size;
  zx_u8 *data;
  zx_u32 *input_ends;
} ZXFuzzInputs;

static inline zx_u32 ZXFuzzInputs_count(ZXFuzzInputs const *inputs) {
  return inputs->n_inputs;
}

static inline void ZXFuzzInputs_select_random(ZXFuzzInputs const *inputs,
                                              ZXXoshiro256 *rand,
                                              zx_u8 const **fuzz_input,
                                              zx_u32 *fuzz_input_len) {
  ZX_ASSERT((inputs->n_inputs > 0));

  zx_u32 const choice =
      zx_rand_mod_u32((zx_u32)ZXXoshiro256_next(rand), inputs->n_inputs);

  zx_u32 const fi_start = inputs->input_ends[choice];
  zx_u32 const fi_end = inputs->input_ends[choice + 1];

  *fuzz_input = &inputs->data[fi_start];
  *fuzz_input_len = zx_chk_sub_u32(fi_end, fi_start);
}

static inline ZXStatus ZXFuzzInputs_init(ZXAlloc *alloc, zx_u32 n_inputs_cap,
                                         zx_u32 data_size_cap,
                                         ZXFuzzInputs *inputs,
                                         ZXErrStack *err_stack) {
  ZXHashtable signatures;
  ZX_ERR_TRY((ZXHashtable_init(alloc, n_inputs_cap, alignof(zx_u64),
                               sizeof(zx_u64), &signatures, err_stack)),
             err_stack);

  inputs->n_inputs_cap = n_inputs_cap;
  inputs->n_inputs = 0;
  inputs->data_size_cap = data_size_cap;
  inputs->data_size = 0;
  inputs->signatures = signatures;

  zx_u8 *data = nullptr;
  ZX_ERR_TRY((ZXAlloc_alloc(alloc, data_size_cap, (void **)&data, err_stack)),
             err_stack);
  inputs->data = data;

  zx_u32 *input_ends = nullptr;
  // allocate one extra since input_ends is like arrow binary-array offsets
  ZX_ERR_TRY((ZXAlloc_alloc(alloc,
                            zx_chk_mul_usize(zx_chk_add_usize(n_inputs_cap, 1),
                                             sizeof(zx_u32)),
                            (void **)&input_ends, err_stack)),
             err_stack);
  input_ends[0] = 0;
  inputs->input_ends = input_ends;

  return ZXStatus_Ok;
}

static inline void ZXFuzzInputs_reset_(ZXFuzzInputs *inputs) {
  ZXHashtable_reset(&inputs->signatures);
  inputs->n_inputs = 0;
  inputs->data_size = 0;
  // not really necessary but trying to make it clear
  inputs->input_ends[0] = 0;
}

static inline bool ZXFuzzInputs_has_enough_cap_(ZXFuzzInputs const *inputs,
                                                zx_u32 fuzz_input_len) {
  bool const have_sig_cap = ZXHashtable_capacity_left(&inputs->signatures) > 0;
  bool const have_data_cap =
      zx_chk_add_u32(inputs->data_size, fuzz_input_len) <=
      inputs->data_size_cap;
  bool const have_n_input_cap =
      zx_chk_add_u32(inputs->n_inputs, 1) <= inputs->n_inputs_cap;
  return have_sig_cap && have_data_cap && have_n_input_cap;
}

static inline bool ZXFuzzInputs_signatures_eql(void const *ctx, void const *a,
                                               void const *b) {
  (void)ctx;
  zx_u64 const *ap = (zx_u64 const *)a;
  zx_u64 const *bp = (zx_u64 const *)b;
  return *ap == *bp;
}

// Resets the state if the new input is unique and we are out of capacity for
// appending it. Returns true if resetted
static inline bool ZXFuzzInputs_append_or_reset(ZXFuzzInputs *inputs,
                                                zx_u8 const *fuzz_input,
                                                zx_u32 fuzz_input_len,
                                                zx_u64 signature) {
  bool did_reset = false;
  // do reset if we don't have enough capacity
  if (!ZXFuzzInputs_has_enough_cap_(inputs, fuzz_input_len)) {
    ZXFuzzInputs_reset_(inputs);
    did_reset = true;
  }

  // Make sure we now have enough capacity
  // There is something really wrong if a single input is bigger than our whole
  // cap.
  ZX_ASSERT((ZXFuzzInputs_has_enough_cap_(inputs, fuzz_input_len)));

  bool const is_uniq = !ZXHashtable_insert(
      &inputs->signatures, signature, &signature, sizeof(signature), nullptr,
      ZXFuzzInputs_signatures_eql);
  if (is_uniq) {
    __builtin_memcpy(&inputs->data[inputs->data_size], fuzz_input,
                     fuzz_input_len);
    inputs->data_size += fuzz_input_len;
    zx_u32 const n_inputs = inputs->n_inputs;
    // write to next since input_ends is like arrow binary-array offsets
    inputs->input_ends[n_inputs + 1] = inputs->data_size;
    inputs->n_inputs = n_inputs + 1;
  }

  return did_reset;
}

#endif

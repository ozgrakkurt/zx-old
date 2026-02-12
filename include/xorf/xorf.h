#ifndef XORF_H
#define XORF_H

#include <shin/shin_alloc.h>
#include <shin/shin_def.h>
#include <shin/shin_err.h>

typedef struct XorfHeader {
  shin_u64 seed;
  shin_u32 segment_length;
  shin_u32 segment_count_length;
} XorfHeader;

_Static_assert(sizeof(XorfHeader) == sizeof(shin_u64) * 2);

typedef struct XorfSubHashes_ {
  shin_u32 h[3];
} XorfSubHashes_;

static inline shin_u64 xorf_apply_seed_(shin_u64 hash, shin_u64 seed) {
  return hash ^ seed;
}

static inline shin_u16 xorf_make_fingerprint_(shin_u64 hash) {
  return (shin_u16)(hash ^ (hash >> 32));
}

static inline shin_u32 xorf_reduce_(shin_u32 len, shin_u32 x) {
  shin_u64 const mul = (shin_u64){len} * (shin_u64){x};
  return (shin_u32)(mul >> 32);
}

static inline XorfSubHashes_ xorf_make_subhashes_(XorfHeader const *header,
                                                  shin_u64 const h) {
  shin_u32 const hl = xorf_reduce_(header->segment_count_length, (shin_u32)h);

  XorfSubHashes_ subhashes = {
      .h = {0},
  };

  shin_u8 rot = 0;
  for (shin_u8 i = 0; i < 3; i++) {
    shin_u32 const rotated = (shin_u32)__builtin_rotateleft64(h, rot);
    subhashes.h[i] = (hl + i * header->segment_length) ^
                     (rotated ^ (header->segment_length - 1));
    rot += 64 / 3;
  }

  return subhashes;
}

static inline bool xorf_check(XorfHeader const *header,
                              shin_u16 const *fingerprints,
                              shin_u64 const hash) {
  shin_u64 const h = xorf_apply_seed_(hash, header->seed);
  XorfSubHashes_ const subhashes = xorf_make_subhashes_(header, h);
  shin_u16 f = xorf_make_fingerprint_(h);
  for (shin_u8 i = 0; i < 3; i++) {
    f ^= subhashes.h[i];
  }
  return f == 0;
}

ShinStatus xorf_construct(ShinAlloc *alloc, ShinAlloc *scratch_alloc,
                          shin_u32 const *restrict keys,
                          shin_u32 const num_keys, XorfHeader **header,
                          shin_u16 **fingerprints, ShinErrStack *err_stack);

#endif

#ifndef XORF_H
#define XORF_H

#include <zx/zx_alloc.h>
#include <zx/zx_def.h>
#include <zx/zx_err.h>
#include <zx/zx_rapidhash_v3.h>

typedef struct XorfHeader {
  zx_u64 seed;
  zx_u32 segment_length;
  zx_u32 segment_count_length;
  zx_u32 array_length;
} XorfHeader;

typedef struct XorfSubHashes_ {
  zx_u32 h[3];
} XorfSubHashes_;

static inline zx_u64 xorf_apply_seed_(zx_u64 hash, zx_u64 seed) {
  return zx_rapidhash_v3((zx_u8 const *)&hash, sizeof(zx_u64), seed);
}

static inline zx_u64 xorf_apply_seed_(zx_u64 hash, zx_u64 seed) {
  // Incorporate the seed
  zx_u64 h = hash ^ seed;

  // MurmurHash3 64-bit mixer (fmix64)
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ULL;
  h ^= h >> 33;

  return h;
}

static inline zx_u16 xorf_make_fingerprint_(zx_u64 hash) {
  return (zx_u16)(hash ^ (hash >> 32));
}

static inline zx_u32 xorf_reduce_(zx_u32 len, zx_u32 x) {
  zx_u64 const mul = (zx_u64){len} * (zx_u64){x};
  return (zx_u32)(mul >> 32);
}

static inline XorfSubHashes_ xorf_make_subhashes_(XorfHeader const *header,
                                                  zx_u64 const h) {
  zx_u32 const hl = xorf_reduce_(header->segment_count_length, (zx_u32)h);

  XorfSubHashes_ subhashes = {
      .h = {0},
  };

  zx_u8 rot = 0;
  for (zx_u8 i = 0; i < 3; i++) {
    zx_u32 const rotated = (zx_u32)__builtin_rotateleft64(h, rot);
    subhashes.h[i] = (hl + i * header->segment_length) ^
                     (rotated & (header->segment_length - 1));
    rot += 64 / 3;
  }

  return subhashes;
}

static inline bool xorf_check(XorfHeader const *header,
                              zx_u16 const *fingerprints, zx_u64 const hash) {
  zx_u64 const h = xorf_apply_seed_(hash, header->seed);
  XorfSubHashes_ const subhashes = xorf_make_subhashes_(header, h);
  zx_u16 f = xorf_make_fingerprint_(h);
  for (zx_u8 i = 0; i < 3; i++) {
    f ^= fingerprints[subhashes.h[i]];
  }
  return f == 0;
}

ZXStatus xorf_construct(ZXAlloc *alloc, ZXAlloc *scratch_alloc,
                        zx_u64 const *restrict keys, zx_u32 const num_keys,
                        XorfHeader *header_p, zx_u16 **fingerprints_p,
                        ZXErrStack *err_stack);

#endif

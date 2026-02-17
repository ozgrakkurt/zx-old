#ifndef KG_ARRAY_H
#define KG_ARRAY_H

#include <shin/shin_def.h>

// Align and pad all array data to this number of bytes.
// padding should always be set to zero.
constexpr shin_u32 KG_ARRAY_ALIGN = 64;

typedef enum KgArrayTag : unsigned char {
  KG_ARRAY_NULL = 0,
  KG_ARRAY_BOOL,
  KG_ARRAY_i8,
  KG_ARRAY_i16,
  KG_ARRAY_i32,
  KG_ARRAY_i64,
  KG_ARRAY_i128,
  KG_ARRAY_i256,
  KG_ARRAY_u8,
  KG_ARRAY_u16,
  KG_ARRAY_u32,
  KG_ARRAY_u64,
  KG_ARRAY_u128,
  KG_ARRAY_u256,
  KG_ARRAY_f16,
  KG_ARRAY_f32,
  KG_ARRAY_f64,
  KG_ARRAY_FS_BIN,
  KG_ARRAY_BIN,
  KG_ARRAY_FS_LIST,
  KG_ARRAY_LIST,
  KG_ARRAY_STRUCT,
  KG_ARRAY_UNION,
  KG_ARRAY_DICT,
  KG_ARRAY_REE,
} KgArrayTag;
_Static_assert(sizeof(KgArrayTag) == 1);

// forward declare for recursion
typedef struct KgArray KgArray;

typedef struct KgArrayPrimitive {
  void const *values;
  void const *validity;
} KgArrayPrimitive;

typedef struct KgArrayFsBin {
  void const *data;
  void const *validity;
  shin_u32 byte_width;
} KgArrayFsBin;

typedef struct KgArrayBin {
  void const *data;
  void const *ends;
  void const *validity;
} KgArrayBin;

typedef struct KgArrayFsList {
  KgArray const *inner;
  void const *validity;
  shin_u32 item_width;
} KgArrayFsList;

typedef struct KgArrayList {
  KgArray const *inner;
  void const *ends;
  void const *validity;
} KgArrayList;

typedef struct KgArrayStruct {
  shin_u8 const *const *field_names;
  KgArray const *const *field_values;
  shin_u8 n_fields;
} KgArrayStruct;

typedef struct KgArrayUnionInner {
  shin_u8 const *const *field_names;
  KgArray const *const *field_values;
  shin_u8 const *type_ids;
  shin_u8 const *types;
} KgArrayUnionInner;
_Static_assert(sizeof(KgArrayUnionInner) == sizeof(void *) * 4);

typedef struct KgArrayUnion {
  KgArrayUnionInner const *inner;
  shin_u8 n_fields;
} KgArrayUnion;

typedef struct KgArrayDict {
  KgArray const *values;
  shin_u32 const *keys;
} KgArrayDict;

typedef struct KgArrayRee {
  KgArray const *values;
  shin_u32 const *run_ends;
} KgArrayRee;

typedef union KgArrayData {
  struct {
  } null;
  KgArrayPrimitive primitive;
  KgArrayFsBin fs_bin;
  KgArrayBin bin;
  KgArrayFsList fs_list;
  KgArrayList list;
  KgArrayStruct struct_;
  KgArrayUnion union_;
  KgArrayDict dict;
  KgArrayRee ree;
} KgArrayData;
_Static_assert(sizeof(KgArrayData) == sizeof(void *) * 3);

struct KgArray {
  KgArrayData data;
  shin_u32 len;
  KgArrayTag tag;
};
_Static_assert(sizeof(KgArray) == 32);

#endif

#ifndef ARRO_ARRAY_H
#define ARRO_ARRAY_H

#include <shin/shin_def.h>

typedef enum AroArrayTag : shin_u8 {
  ARO_ARRAY_NULL,
  ARO_ARRAY_i8,
  ARO_ARRAY_i16,
  ARO_ARRAY_i32,
  ARO_ARRAY_i64,
  ARO_ARRAY_i128,
  ARO_ARRAY_i256,
  ARO_ARRAY_u8,
  ARO_ARRAY_u16,
  ARO_ARRAY_u32,
  ARO_ARRAY_u64,
  ARO_ARRAY_u128,
  ARO_ARRAY_u256,
  ARO_ARRAY_f16,
  ARO_ARRAY_f32,
  ARO_ARRAY_f64,
  ARO_ARRAY_BINARY,
  ARO_ARRAY_BOOL,
  ARO_ARRAY_LIST,
  ARO_ARRAY_STRUCT,
  ARO_ARRAY_ENUM,
} AroArrayTag;
_Static_assert(sizeof(AroArrayTag) == 1);

typedef union AroArrayData {

} AroArrayData;
_Static_assert(sizeof(AroArrayData) == 69);

typedef struct AroArray {
  AroArrayTag tag;
  AroArrayData data;
} AroArray;

#endif

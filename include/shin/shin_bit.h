#ifndef SHIN_BIT_H
#define SHIN_BIT_H

#include <shin/shin_def.h>
#include <shin/shin_err.h>

#define SHIN_CTZ(x)                                                            \
  ({                                                                           \
    auto const shin_ctz_val_ = (x);                                            \
    constexpr int SHIN_CTZ_BIT_SIZE = sizeof(shin_ctz_val_) * 8;               \
                                                                               \
    (unsigned int)__builtin_ctzg(shin_ctz_val_, SHIN_CTZ_BIT_SIZE);            \
  })

#define SHIN_CLZ(x)                                                            \
  ({                                                                           \
    auto const shin_clz_val_ = (x);                                            \
    constexpr int SHIN_CLZ_BIT_SIZE = sizeof(shin_clz_val_) * 8;               \
                                                                               \
    (unsigned int)__builtin_clzg(shin_clz_val_, SHIN_CLZ_BIT_SIZE);            \
  })

#define SHIN_IS_POWER_OF_TWO(x) ((bool)(__builtin_popcountg(x) == 1))

#define SHIN_BIT_WIDTH(x)                                                      \
  ({                                                                           \
    auto const shin_bit_width_val_ = (x);                                      \
    constexpr unsigned int SHIN_BIT_WIDTH_BIT_SIZE =                           \
        sizeof(shin_bit_width_val_) * 8;                                       \
                                                                               \
    (unsigned int)(SHIN_BIT_WIDTH_BIT_SIZE - SHIN_CLZ(shin_bit_width_val_));   \
  })

#define SHIN_NEXT_POWER_OF_TWO(x)                                              \
  ({                                                                           \
    auto const shin_next_power_of_two_val_ = (x);                              \
    auto const shin_next_power_of_two_limit_ =                                 \
        SHIN_UINT_MAX(typeof(shin_next_power_of_two_val_)) >> 1;               \
    SHIN_ASSERT(                                                               \
        (shin_next_power_of_two_val_ <= shin_next_power_of_two_limit_));       \
                                                                               \
    typeof(shin_next_power_of_two_val_) const shin_next_power_of_two_res_ =    \
        shin_next_power_of_two_val_ <= 1                                       \
            ? ((typeof(shin_next_power_of_two_val_)){1wbu})                    \
            : (typeof(shin_next_power_of_two_val_)){2wbu}                      \
                  << (SHIN_BIT_WIDTH((shin_next_power_of_two_val_ - 1wbu)) -   \
                      1wbu);                                                   \
    shin_next_power_of_two_res_;                                               \
  })

#endif

#pragma once

#include "math.h"

#if TR_VERSION == 1
typedef struct __PACKING {
    int32_t mid_floor;
    int32_t mid_ceiling;
    int32_t mid_type;
    int32_t front_floor;
    int32_t front_ceiling;
    int32_t front_type;
    int32_t left_floor;
    int32_t left_ceiling;
    int32_t left_type;
    int32_t right_floor;
    int32_t right_ceiling;
    int32_t right_type;
    int32_t radius;
    int32_t bad_pos;
    int32_t bad_neg;
    int32_t bad_ceiling;
    XYZ_32 shift;
    XYZ_32 old;
    int16_t facing;
    DIRECTION quadrant;
    int16_t coll_type;
    int8_t tilt_x;
    int8_t tilt_z;
    int8_t hit_by_baddie;
    int8_t hit_static;
    uint16_t slopes_are_walls : 1;
    uint16_t slopes_are_pits : 1;
    uint16_t lava_is_pit : 1;
    uint16_t enable_baddie_push : 1;
    uint16_t enable_spaz : 1;
} COLL_INFO;

#elif TR_VERSION == 2
typedef struct __PACKING {
    int32_t floor;
    int32_t ceiling;
    int32_t type;
} COLL_SIDE;

typedef struct __PACKING {
    COLL_SIDE side_mid;
    COLL_SIDE side_front;
    COLL_SIDE side_left;
    COLL_SIDE side_right;
    int32_t radius;
    int32_t bad_pos;
    int32_t bad_neg;
    int32_t bad_ceiling;
    XYZ_32 shift;
    XYZ_32 old;
    int16_t old_anim_state;
    int16_t old_anim_num;
    int16_t old_frame_num;
    int16_t facing;
    int16_t quadrant;
    int16_t coll_type;
    int16_t *trigger;
    int8_t x_tilt;
    int8_t z_tilt;
    int8_t hit_by_baddie;
    int8_t hit_static;
    // clang-format off
    uint16_t slopes_are_walls:   1; // 0x01 1
    uint16_t slopes_are_pits:    1; // 0x02 2
    uint16_t lava_is_pit:        1; // 0x04 4
    uint16_t enable_baddie_push: 1; // 0x08 8
    uint16_t enable_spaz:        1; // 0x10 16
    uint16_t hit_ceiling:        1; // 0x20 32
    uint16_t pad:                10;
    // clang-format on
} COLL_INFO;
#endif

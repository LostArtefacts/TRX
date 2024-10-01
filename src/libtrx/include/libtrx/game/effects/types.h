#pragma once

#include "../math.h"
#include "../objects/ids.h"
#include "../types.h"

typedef struct __PACKING {
    XYZ_32 pos;
    XYZ_16 rot;
    int16_t room_num;
#if TR_VERSION == 1
    GAME_OBJECT_ID object_id;
#else
    int16_t object_id;
#endif
#if TR_VERSION == 1
    int16_t next_draw;
#endif
    int16_t next_free;
    int16_t next_active;
    int16_t speed;
    int16_t fall_speed;
    int16_t frame_num;
    int16_t counter;
    int16_t shade;

#if TR_VERSION == 1
    struct __PACKING {
        struct __PACKING {
            XYZ_32 pos;
            XYZ_16 rot;
        } result, prev;
    } interp;
#endif
} FX;

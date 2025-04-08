#pragma once

#include "../items.h"
#include "../pathing.h"
#include "./enum.h"

typedef struct {
    int16_t head_rotation;
    int16_t neck_rotation;
    int16_t maximum_turn;
    int16_t flags;
    int16_t item_num;
    MOOD_TYPE mood;
    LOT_INFO lot;
    XYZ_32 target;
    ITEM *enemy;
} CREATURE;

typedef struct {
    int16_t zone_num;
    // TODO: merge
    union {
        int16_t enemy_zone;
        int16_t enemy_zone_num;
    };
    int32_t distance;
    int32_t ahead;
    int32_t bite;
    int16_t angle;
    int16_t enemy_facing;
} AI_INFO;

typedef struct {
    // TODO: merge
#if TR_VERSION == 1
    int32_t x;
    int32_t y;
    int32_t z;
#else
    union {
        struct {
            int32_t x, y, z;
        };
        XYZ_32 pos;
    };
#endif
    int32_t mesh_num;
} BITE;

typedef struct {
    struct {
        GAME_OBJECT_ID id;
        int16_t active_anim;
        int16_t death_anim;
        int16_t death_state;
    } land;
    struct {
        GAME_OBJECT_ID id;
        int16_t active_anim;
    } water;
} HYBRID_INFO;

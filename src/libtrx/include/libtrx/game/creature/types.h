#pragma once

#include "../items.h"
#include "../pathing/types.h"
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
    XYZ_32 pos;
    int32_t mesh_num;
} BITE;

typedef struct {
    struct {
        OBJECT_ID id;
        int16_t active_anim;
        int16_t death_anim;
        int16_t death_state;
    } land, water;
} HYBRID_INFO;

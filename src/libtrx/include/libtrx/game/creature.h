#pragma once

#include "items.h"
#include "math.h"
#include "pathing.h"

#include <stdint.h>

typedef enum {
    MOOD_BORED = 0,
    MOOD_ATTACK = 1,
    MOOD_ESCAPE = 2,
    MOOD_STALK = 3,
} MOOD_TYPE;

typedef struct {
    int16_t head_rotation;
    int16_t neck_rotation;
    int16_t maximum_turn;
    int16_t flags;
    int16_t item_num;
    MOOD_TYPE mood;
    LOT_INFO lot;
    XYZ_32 target;
#if TR_VERSION == 2
    ITEM *enemy;
#endif
} CREATURE;

#define CREATURE_STALK_DIST (3 * WALL_L) // = 3072
#define CREATURE_ESCAPE_DIST (5 * WALL_L) // = 5120
#define CREATURE_TARGET_DIST (4 * WALL_L) // = 4096

bool Creature_IsHostile(const ITEM *item);

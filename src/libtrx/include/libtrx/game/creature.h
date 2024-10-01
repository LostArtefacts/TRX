#pragma once

#include "items.h"
#include "lot.h"
#include "math.h"

#include <stdint.h>

typedef enum {
    MOOD_BORED = 0,
    MOOD_ATTACK = 1,
    MOOD_ESCAPE = 2,
    MOOD_STALK = 3,
} MOOD_TYPE;

typedef struct __PACKING {
    int16_t head_rotation;
    int16_t neck_rotation;
    int16_t maximum_turn;
    uint16_t flags;
    int16_t item_num;
    MOOD_TYPE mood;
    LOT_INFO lot;
    XYZ_32 target;
#if TR_VERSION == 2
    ITEM *enemy;
#endif
} CREATURE;

bool Creature_IsEnemy(const ITEM *item);

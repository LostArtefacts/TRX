#pragma once

#include <libtrx/game/item_actions.h>

typedef enum {
    // clang-format off
    ITEM_ACTION_TURN_180            = 0,
    ITEM_ACTION_FLOOR_SHAKE         = 1,
    ITEM_ACTION_LARA_NORMAL         = 2,
    ITEM_ACTION_BUBBLES             = 3,
    ITEM_ACTION_FINISH_LEVEL        = 4,
    ITEM_ACTION_EARTHQUAKE          = 5,
    ITEM_ACTION_FLOOD               = 6,
    ITEM_ACTION_RAISING_BLOCK       = 7,
    ITEM_ACTION_STAIRS_TO_SLOPE     = 8,
    ITEM_ACTION_DROP_SAND           = 9,
    ITEM_ACTION_POWER_UP            = 10,
    ITEM_ACTION_EXPLOSION           = 11,
    ITEM_ACTION_LARA_HANDS_FREE     = 12,
    ITEM_ACTION_FLIP_MAP            = 13,
    ITEM_ACTION_LARA_DRAW_RIGHT_GUN = 14,
    ITEM_ACTION_CHAIN_BLOCK         = 15,
    ITEM_ACTION_FLICKER             = 16,
    // clang-format on
} ITEM_ACTION;

void ItemAction_RunActive(void);

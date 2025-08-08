#pragma once

#include "./items.h"

typedef enum {
    // clang-format off
    ITEM_ACTION_TURN_180                     = 0,
    ITEM_ACTION_FLOOR_SHAKE                  = 1,
    ITEM_ACTION_LARA_NORMAL                  = 2,
    ITEM_ACTION_BUBBLES                      = 3,
    ITEM_ACTION_FINISH_LEVEL                 = 4,
    ITEM_ACTION_EXPLOSION                    = 11,
    ITEM_ACTION_LARA_HANDS_FREE              = 12,
    ITEM_ACTION_FLIP_MAP                     = 13,
    ITEM_ACTION_LARA_DRAW_RIGHT_GUN          = 14,
#if TR_VERSION == 1
    ITEM_ACTION_EARTHQUAKE                   = 5,
    ITEM_ACTION_FLOOD                        = 6,
    ITEM_ACTION_RAISING_BLOCK                = 7,
    ITEM_ACTION_STAIRS_TO_SLOPE              = 8,
    ITEM_ACTION_DROP_SAND                    = 9,
    ITEM_ACTION_POWER_UP                     = 10,
    ITEM_ACTION_CHAIN_BLOCK                  = 15,
    ITEM_ACTION_FLICKER                      = 16,
#elif TR_VERSION == 2
    ITEM_ACTION_FLOOD                        = 5,
    ITEM_ACTION_CHANDELIER                   = 6,
    ITEM_ACTION_RUBBLE                       = 7,
    ITEM_ACTION_PISTON                       = 8,
    ITEM_ACTION_CURTAIN                      = 9,
    ITEM_ACTION_SET_CHANGE                   = 10,
    ITEM_ACTION_LARA_DRAW_LEFT_GUN           = 15,
    // missing 16
    // missing 17
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_1 = 18,
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_2 = 19,
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_3 = 20,
    ITEM_ACTION_INVISIBILITY_ON              = 21,
    ITEM_ACTION_INVISIBILITY_OFF             = 22,
    ITEM_ACTION_DYNAMIC_LIGHT_ON             = 23,
    ITEM_ACTION_DYNAMIC_LIGHT_OFF            = 24,
    ITEM_ACTION_STATUE                       = 25,
    ITEM_ACTION_RESET_HAIR                   = 26,
    ITEM_ACTION_BOILER                       = 27,
    ITEM_ACTION_ASSAULT_RESET                = 28,
    ITEM_ACTION_ASSAULT_STOP                 = 29,
    ITEM_ACTION_ASSAULT_START                = 30,
    ITEM_ACTION_ASSAULT_FINISHED             = 31,
// clang-format on
#endif
} ITEM_ACTION;

extern void ItemAction_Run(int16_t action_id, ITEM *item);

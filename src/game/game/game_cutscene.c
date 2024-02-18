#include "game/game.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_hair.h"
#include "game/level.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdint.h>

static void Game_Cutscene_InitialiseHair(int32_t level_num);

static void Game_Cutscene_InitialiseHair(int32_t level_num)
{
    if (!g_Config.enable_braid || !g_Objects[O_HAIR].loaded) {
        return;
    }

    GAME_OBJECT_ID lara_type = g_GameFlow.levels[level_num].lara_type;
    if (lara_type == O_LARA) {
        return;
    }

    int16_t lara_item_num = NO_ITEM;
    for (int i = 0; i < g_LevelItemCount; i++) {
        if (g_Items[i].object_number == lara_type) {
            lara_item_num = i;
            break;
        }
    }

    if (lara_item_num == NO_ITEM) {
        return;
    }

    Lara_InitialiseLoad(lara_item_num);
    Lara_Initialise(level_num);
    Lara_Hair_SetLaraType(lara_type);

    Item_SwitchToObjAnim(g_LaraItem, 0, 0, lara_type);
    ANIM_STRUCT *cut_anim = &g_Anims[g_LaraItem->anim_number];
    g_LaraItem->current_anim_state = g_LaraItem->goal_anim_state =
        g_LaraItem->required_anim_state = cut_anim->current_anim_state;
}

GAMEFLOW_OPTION Game_Cutscene_Start(const int32_t level_num)
{
    if (!Level_Initialise(level_num)) {
        return GF_NOP_BREAK;
    }

    Game_Cutscene_InitialiseHair(level_num);

    for (int16_t room_num = 0; room_num < g_RoomCount; room_num++) {
        if (g_RoomInfo[room_num].flipped_room >= 0) {
            g_RoomInfo[g_RoomInfo[room_num].flipped_room].bound_active = 1;
        }
    }

    g_RoomsToDrawCount = 0;
    for (int16_t room_num = 0; room_num < g_RoomCount; room_num++) {
        if (!g_RoomInfo[room_num].bound_active) {
            if (g_RoomsToDrawCount + 1 < MAX_ROOMS_TO_DRAW) {
                g_RoomsToDraw[g_RoomsToDrawCount++] = room_num;
            }
        }
    }

    g_CineFrame = 0;
    return GF_NOP;
}

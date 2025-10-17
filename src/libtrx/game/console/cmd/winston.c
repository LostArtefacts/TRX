#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/const.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/music.h"
#include "game/objects/vars.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "memory.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!lara_item->hit_points) {
        return CR_UNAVAILABLE;
    }

    const OBJECT *const obj = Object_Get(O_WINSTON);
    if (!obj->loaded) {
        return CR_UNAVAILABLE;
    }

    int16_t target_room_num = lara_item->room_num;
    XYZ_32 target_pos = {
        .x = lara_item->pos.x + STEP_L,
        .y = lara_item->pos.y - WALL_L,
        .z = lara_item->pos.z + STEP_L,
    };
    if (!Room_FindValidPos(&target_pos, &target_room_num)) {
        Console_LogError(GS(CMD_WINSTON_SPAWN_FAILED));
        return CR_FAILURE;
    }

    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->killed_loyal_item) {
        Music_Stop();
        Console_Log(GS(CMD_WINSTON_DEAD));
        return CR_FAILURE;
    }

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id == O_WINSTON) {
            if (item->status == IS_INVISIBLE || item->status == IS_INACTIVE) {
                item->status = IS_ACTIVE;
                Item_AddActive(item_num);
                LOT_EnableBaddieAI(item_num, true);
            } else if ((item->flags & IF_KILLED) != 0) {
                Music_Stop();
                Console_Log(GS(CMD_WINSTON_DEAD));
                return CR_FAILURE;
            }
            item->pos.x = target_pos.x;
            item->pos.y = target_pos.y;
            item->pos.z = target_pos.z;
            item->rot.y = lara_item->rot.y;
            Item_UpdateRoom(item_num, target_room_num);
            Console_Log(GS(CMD_WINSTON_TELEPORTED));
            return CR_SUCCESS;
        }
    }

    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return CR_FAILURE;
    }
    ITEM *const new_item = Item_Get(item_num);
    new_item->object_id = O_WINSTON;
    new_item->room_num = target_room_num;
    new_item->pos.x = target_pos.x;
    new_item->pos.y = target_pos.y;
    new_item->pos.z = target_pos.z;
    new_item->rot.y = lara_item->rot.y;
    new_item->shade.value_1 = -1;
    Item_Initialise(item_num);
    Item_AddActive(item_num);
    new_item->status = IS_ACTIVE;
    LOT_EnableBaddieAI(item_num, true);

    Console_Log(GS(CMD_WINSTON_SPAWNED));
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("teatime", M_Entrypoint, nullptr)

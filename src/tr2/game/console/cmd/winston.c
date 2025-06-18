#include <libtrx/game/console/common.h>
#include <libtrx/game/console/registry.h>
#include <libtrx/game/const.h>
#include <libtrx/game/creature.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_string.h>
#include <libtrx/game/items.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/pathing.h>
#include <libtrx/game/rooms.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!lara_item->hit_points) {
        return CR_UNAVAILABLE;
    }

    int16_t target_room_num = lara_item->room_num;
    XYZ_32 target_pos = {
        .x = lara_item->pos.x + STEP_L,
        .y = lara_item->pos.y - WALL_L,
        .z = lara_item->pos.z + STEP_L,
    };
    if (!Room_FindValidPos(&target_pos, &target_room_num)) {
        Console_Log(GS(CMD_WINSTON_SPAWN_FAILED));
        return CR_FAILURE;
    }

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id == O_WINSTON) {
            if (!Creature_IsAlive(item)) {
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

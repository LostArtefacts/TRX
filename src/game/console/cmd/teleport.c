#include "game/console/cmd/teleport.h"

#include "game/game_string.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/strings.h>

#include <math.h>
#include <stdio.h>

static bool M_IsFloatRound(const float num);
;
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static inline bool M_IsFloatRound(const float num)
{
    return (fabsf(num) - roundf(num)) < 0.0001f;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (g_GameInfo.current_level_type == GFL_TITLE
        || g_GameInfo.current_level_type == GFL_DEMO
        || g_GameInfo.current_level_type == GFL_CUTSCENE) {
        return CR_UNAVAILABLE;
    }

    if (!g_Objects[O_LARA].loaded || !g_LaraItem->hit_points) {
        return CR_UNAVAILABLE;
    }

    // X Y Z
    {
        float x, y, z;
        if (sscanf(ctx->args, "%f %f %f", &x, &y, &z) == 3) {
            if (M_IsFloatRound(x)) {
                x += 0.5f;
            }
            if (M_IsFloatRound(z)) {
                z += 0.5f;
            }

            if (Lara_Cheat_Teleport(x * WALL_L, y * WALL_L, z * WALL_L)) {
                Console_Log(GS(OSD_POS_SET_POS), x, y, z);
                return CR_SUCCESS;
            }

            Console_Log(GS(OSD_POS_SET_POS_FAIL), x, y, z);
            return CR_FAILURE;
        }
    }

    // Room number
    {
        int16_t room_num = NO_ROOM;
        if (sscanf(ctx->args, "%hd", &room_num) == 1) {
            if (room_num < 0 || room_num >= g_RoomCount) {
                Console_Log(GS(OSD_INVALID_ROOM), room_num, g_RoomCount - 1);
                return CR_SUCCESS;
            }

            const ROOM_INFO *const room = &g_RoomInfo[room_num];

            const int32_t x1 = room->x + WALL_L;
            const int32_t x2 = (room->x_size << WALL_SHIFT) + room->x - WALL_L;
            const int32_t y1 = room->min_floor;
            const int32_t y2 = room->max_ceiling;
            const int32_t z1 = room->z + WALL_L;
            const int32_t z2 = (room->z_size << WALL_SHIFT) + room->z - WALL_L;

            for (int i = 0; i < 100; i++) {
                int32_t x = x1 + Random_GetControl() * (x2 - x1) / 0x7FFF;
                int32_t y = y1;
                int32_t z = z1 + Random_GetControl() * (z2 - z1) / 0x7FFF;
                if (Lara_Cheat_Teleport(x, y, z)) {
                    Console_Log(GS(OSD_POS_SET_ROOM), room_num);
                    return CR_SUCCESS;
                }
            }

            Console_Log(GS(OSD_POS_SET_ROOM_FAIL), room_num);
            return CR_FAILURE;
        }
    }

    // Nearest item of this name
    if (!String_Equivalent(ctx->args, "")) {
        int32_t match_count = 0;
        GAME_OBJECT_ID *matching_objs =
            Object_IdsFromName(ctx->args, &match_count, NULL);

        const ITEM_INFO *best_item = NULL;
        int32_t best_distance = INT32_MAX;

        for (int16_t item_num = 0; item_num < Item_GetTotalCount();
             item_num++) {
            const ITEM_INFO *const item = &g_Items[item_num];
            const bool is_consumable =
                Object_IsObjectType(item->object_id, g_PickupObjects)
                || item->object_id == O_SAVEGAME_ITEM;
            if (is_consumable
                && (item->status == IS_INVISIBLE
                    || item->status == IS_DEACTIVATED)) {
                continue;
            }

            if (item->flags & IF_KILLED) {
                continue;
            }

            bool is_matched = false;
            for (int32_t i = 0; i < match_count; i++) {
                if (matching_objs[i] == item->object_id) {
                    is_matched = true;
                    break;
                }
            }
            if (!is_matched) {
                continue;
            }

            const int32_t distance = Item_GetDistance(item, &g_LaraItem->pos);
            if (distance < best_distance) {
                best_distance = distance;
                best_item = item;
            }
        }

        if (best_item != NULL) {
            if (Lara_Cheat_Teleport(
                    best_item->pos.x, best_item->pos.y, best_item->pos.z)) {
                Console_Log(GS(OSD_POS_SET_ITEM), ctx->args);
            } else {
                Console_Log(GS(OSD_POS_SET_ITEM_FAIL), ctx->args);
            }
            return CR_SUCCESS;
        } else {
            Console_Log(GS(OSD_POS_SET_ITEM_FAIL), ctx->args);
            return CR_FAILURE;
        }
    }

    return CR_BAD_INVOCATION;
}

CONSOLE_COMMAND g_Console_Cmd_Teleport = {
    .prefix = "tp",
    .proc = M_Entrypoint,
};

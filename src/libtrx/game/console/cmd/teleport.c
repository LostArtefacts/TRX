#include "game/console/cmd/teleport.h"

#include "game/const.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/objects/vars.h"
#include "game/random.h"
#include "game/rooms.h"
#include "strings.h"

#include <math.h>
#include <stdio.h>

static bool M_CanTargetObject(GAME_OBJECT_ID object_id);
static bool M_IsFloatRound(float num);

static COMMAND_RESULT M_TeleportToXYZ(float x, float y, float z);
static COMMAND_RESULT M_TeleportToRoom(int16_t room_num);
static COMMAND_RESULT M_TeleportToObject(const char *user_input);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static bool M_CanTargetObject(const GAME_OBJECT_ID object_id)
{
    return !Object_IsObjectType(object_id, g_NullObjects)
        && !Object_IsObjectType(object_id, g_AnimObjects)
        && !Object_IsObjectType(object_id, g_InvObjects);
}

static inline bool M_IsFloatRound(const float num)
{
    return (fabsf(num) - roundf(num)) < 0.0001f;
}

static COMMAND_RESULT M_TeleportToXYZ(float x, const float y, float z)
{
    if (M_IsFloatRound(x)) {
        x += 0.5f;
    }
    if (M_IsFloatRound(z)) {
        z += 0.5f;
    }

    if (!Lara_Cheat_Teleport(x * WALL_L, y * WALL_L, z * WALL_L)) {
        Console_Log(GS(OSD_POS_SET_POS_FAIL), x, y, z);
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_POS_SET_POS), x, y, z);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_TeleportToRoom(const int16_t room_num)
{
    if (room_num < 0 || room_num >= Room_GetTotalCount()) {
        Console_Log(GS(OSD_INVALID_ROOM), room_num, Room_GetTotalCount() - 1);
        return CR_FAILURE;
    }

    const ROOM *const room = Room_Get(room_num);
    const int32_t x1 = room->pos.x + WALL_L;
    const int32_t x2 = room->pos.x + (room->size.x << WALL_SHIFT) - WALL_L;
    const int32_t y1 = room->min_floor;
    const int32_t y2 = room->max_ceiling;
    const int32_t z1 = room->pos.z + WALL_L;
    const int32_t z2 = room->pos.z + (room->size.z << WALL_SHIFT) - WALL_L;

    bool success = false;
    for (int32_t i = 0; i < 100; i++) {
        int32_t x = x1 + Random_GetControl() * (x2 - x1) / 0x7FFF;
        int32_t y = y1;
        int32_t z = z1 + Random_GetControl() * (z2 - z1) / 0x7FFF;
        if (Lara_Cheat_Teleport(x, y, z)) {
            success = true;
            break;
        }
    }

    if (!success) {
        Console_Log(GS(OSD_POS_SET_ROOM_FAIL), room_num);
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_POS_SET_ROOM), room_num);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_TeleportToObject(const char *const user_input)
{
    // Nearest item of this name
    if (String_Equivalent(user_input, "")) {
        return CR_BAD_INVOCATION;
    }

    int32_t match_count = 0;
    GAME_OBJECT_ID *matching_objs =
        Object_IdsFromName(user_input, &match_count, M_CanTargetObject);

    const ITEM *const lara_item = Lara_GetItem();
    const ITEM *best_item = NULL;
    int32_t best_distance = INT32_MAX;

    for (int16_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (Object_IsObjectType(item->object_id, g_PickupObjects)
            && (item->status == IS_INVISIBLE || item->status == IS_DEACTIVATED
                || item->room_num == NO_ROOM)) {
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

        const int32_t distance = Item_GetDistance(item, &lara_item->pos);
        if (distance < best_distance) {
            best_distance = distance;
            best_item = item;
        }
    }

    if (best_item == NULL) {
        Console_Log(GS(OSD_POS_SET_ITEM_FAIL), user_input);
        return CR_FAILURE;
    }

    const char *obj_name = Object_GetName(best_item->object_id);
    if (obj_name == NULL) {
        obj_name = user_input;
    }

    if (Lara_Cheat_Teleport(
            best_item->pos.x, best_item->pos.y - STEP_L, best_item->pos.z)) {
        Console_Log(GS(OSD_POS_SET_ITEM), obj_name);
    } else {
        Console_Log(GS(OSD_POS_SET_ITEM_FAIL), obj_name);
    }
    return CR_SUCCESS;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (!lara_item->hit_points) {
        return CR_UNAVAILABLE;
    }

    float x, y, z;
    if (sscanf(ctx->args, "%f %f %f", &x, &y, &z) == 3) {
        return M_TeleportToXYZ(x, y, z);
    }

    int16_t room_num = -1;
    if (sscanf(ctx->args, "%hd", &room_num) == 1) {
        return M_TeleportToRoom(room_num);
    }

    return M_TeleportToObject(ctx->args);
}

CONSOLE_COMMAND g_Console_Cmd_Teleport = {
    .prefix = "tp",
    .proc = M_Entrypoint,
};

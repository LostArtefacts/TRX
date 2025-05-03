#include "game/console/common.h"
#include "game/console/registry.h"
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
#include "memory.h"
#include "strings.h"

#include <math.h>
#include <stdio.h>

static int16_t m_LastTeleportedItemNum = NO_ITEM;

static bool M_CanTargetObject(GAME_OBJECT_ID obj_id);
static bool M_CanTargetItem(
    const ITEM *item, const GAME_OBJECT_ID *matching_objs, int32_t match_count);
static const ITEM *M_GetItemToTeleporTo(const char *user_input);
static bool M_IsFloatRound(float num);

static COMMAND_RESULT M_TeleportToXYZ(float x, float y, float z);
static COMMAND_RESULT M_TeleportToRoom(int16_t room_num);
static COMMAND_RESULT M_TeleportToObject(const char *user_input);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static bool M_ObjectCanBePickedUp(const GAME_OBJECT_ID obj_id)
{
    if (!Object_IsType(obj_id, g_PickupObjects)) {
        return true;
    }
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (item->object_id == obj_id && item->status != IS_INVISIBLE) {
            return true;
        }
    }
    return false;
}

static bool M_CanTargetObject(const GAME_OBJECT_ID obj_id)
{
    return !Object_IsType(obj_id, g_NullObjects)
        && !Object_IsType(obj_id, g_AnimObjects)
        && !Object_IsType(obj_id, g_InvObjects) && Object_Get(obj_id)->loaded
        && M_ObjectCanBePickedUp(obj_id);
}

static bool M_CanTargetItem(
    const ITEM *const item, const GAME_OBJECT_ID *const matching_objs,
    const int32_t match_count)
{
    // Collected pickups
    if (Object_IsType(item->object_id, g_PickupObjects)
        && (item->status == IS_INVISIBLE || item->status == IS_DEACTIVATED
            || item->room_num == NO_ROOM)) {
        return false;
    }

    // Killed enemies and removed items
    if (item->flags & IF_KILLED) {
        return false;
    }

    // Non-matches to user input
    bool is_matched = false;
    for (int32_t j = 0; j < match_count; j++) {
        if (matching_objs[j] == item->object_id) {
            is_matched = true;
            break;
        }
    }
    if (!is_matched) {
        return false;
    }

    return true;
}

static const ITEM *M_GetItemToTeleporTo(const char *const user_input)
{
    int32_t match_count = 0;
    GAME_OBJECT_ID *matching_objs =
        Object_IdsFromName(user_input, &match_count, M_CanTargetObject);

    const ITEM *const lara_item = Lara_GetItem();

    int16_t best_item_num = NO_ITEM;

    // Choose the matching item closest to Lara.
    const int32_t near_distance = WALL_L;
    int16_t closest_item_num = NO_ITEM;
    int32_t closest_distance = INT32_MAX;
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);
        if (!M_CanTargetItem(item, matching_objs, match_count)) {
            continue;
        }

        const int32_t distance = Item_GetDistance(item, &lara_item->pos);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest_item_num = item_num;
        }
    }

    if (closest_distance > near_distance) {
        best_item_num = closest_item_num;
    } else {
        // If Lara's already very close to a matching item, choose the next
        // matching item in a round-robin fashion.
        const int16_t start_idx = (closest_item_num + 1) % Item_GetTotalCount();
        for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
            int16_t item_num = (start_idx + i) % Item_GetTotalCount();
            if (item_num == closest_item_num) {
                continue;
            }

            const ITEM *const item = Item_Get(item_num);
            if (!M_CanTargetItem(item, matching_objs, match_count)) {
                continue;
            }

            const int32_t distance = Item_GetDistance(item, &lara_item->pos);
            if (distance < near_distance) {
                continue;
            }

            best_item_num = item_num;
            break;
        }
    }

    Memory_FreePointer(&matching_objs);
    return Item_Get(best_item_num);
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

    if (!Lara_Cheat_Teleport(x * WALL_L, y * WALL_L, z * WALL_L, NO_ROOM)) {
        Console_Log(GS(OSD_POS_SET_POS_FAIL), x, y, z);
        return CR_FAILURE;
    }

    Console_Log(GS(OSD_POS_SET_POS), x, y, z);
    return CR_SUCCESS;
}

static COMMAND_RESULT M_TeleportToRoom(const int16_t room_num)
{
    if (room_num < 0 || room_num >= Room_GetCount()) {
        Console_Log(GS(OSD_INVALID_ROOM), room_num, Room_GetCount() - 1);
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
        if (Lara_Cheat_Teleport(x, y, z, room_num)) {
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

    const ITEM *const best_item = M_GetItemToTeleporTo(user_input);
    if (best_item == nullptr) {
        Console_Log(GS(OSD_POS_SET_ITEM_FAIL), user_input);
        return CR_FAILURE;
    }

    const char *obj_name = Object_GetName(best_item->object_id);
    if (obj_name == nullptr) {
        obj_name = user_input;
    }

    if (Lara_Cheat_Teleport(
            best_item->pos.x, best_item->pos.y - STEP_L / 4, best_item->pos.z,
            best_item->room_num)) {
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

REGISTER_CONSOLE_COMMAND("tp", M_Entrypoint, GS_ID(CONSOLE_HELP_TP))

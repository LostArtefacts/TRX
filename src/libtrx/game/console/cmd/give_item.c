#include "game/console/cmd/give_item.h"

#include "game/backpack.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/lara/cheat.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/objects/vars.h"
#include "memory.h"
#include "strings.h"

#include <stdio.h>
#include <string.h>

static bool M_CanTargetObjectPickup(GAME_OBJECT_ID object_id);
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static bool M_CanTargetObjectPickup(const GAME_OBJECT_ID object_id)
{
    return Object_IsObjectType(object_id, g_PickupObjects)
        && Object_GetObject(object_id)->loaded;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    if (String_Equivalent(ctx->args, "keys")) {
        return Lara_Cheat_GiveAllKeys() ? CR_SUCCESS : CR_FAILURE;
    }

    if (String_Equivalent(ctx->args, "guns")) {
        return Lara_Cheat_GiveAllGuns() ? CR_SUCCESS : CR_FAILURE;
    }

    if (String_Equivalent(ctx->args, "all")) {
        return Lara_Cheat_GiveAllItems() ? CR_SUCCESS : CR_FAILURE;
    }

    int32_t num = 1;
    const char *args = ctx->args;
    if (sscanf(ctx->args, "%d ", &num) == 1) {
        args = strstr(args, " ");
        if (args == NULL) {
            return CR_BAD_INVOCATION;
        }
        args++;
    }

    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    bool found = false;
    int32_t match_count = 0;
    GAME_OBJECT_ID *matching_objs =
        Object_IdsFromName(args, &match_count, M_CanTargetObjectPickup);
    for (int32_t i = 0; i < match_count; i++) {
        const GAME_OBJECT_ID object_id = matching_objs[i];
        if (Object_GetObject(object_id)->loaded) {
            const char *obj_name = Object_GetName(object_id);
            if (obj_name == NULL) {
                obj_name = args;
            }
            Backpack_AddItemNTimes(object_id, num);
            Console_Log(GS(OSD_GIVE_ITEM), obj_name);
            found = true;
        }
    }
    Memory_FreePointer(&matching_objs);

    if (!found) {
        Console_Log(GS(OSD_INVALID_ITEM), args);
        return CR_FAILURE;
    }

    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_GiveItem = {
    .prefix = "give",
    .proc = M_Entrypoint,
};

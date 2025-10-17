#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara/cheat.h"
#include "game/objects/common.h"
#include "game/objects/names.h"
#include "game/objects/vars.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static bool M_CanTargetObjectPickup(const OBJECT_ID obj_id)
{
    return Object_IsType(obj_id, g_InvObjects) && Object_Get(obj_id)->loaded
        && Object_IsType(
               Object_GetCognateInverse(obj_id, g_ItemToInvObjectMap),
               g_PickupObjects);
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
        if (args == nullptr) {
            return CR_BAD_INVOCATION;
        }
        CLAMPG(num, MAX_QTY);
        args++;
    }

    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    bool found = false;
    int32_t match_count = 0;
    OBJECT_NAME_MATCH *matches =
        Object_IdsFromName(args, &match_count, M_CanTargetObjectPickup);
    for (int32_t i = 0; i < match_count; i++) {
        const OBJECT_ID obj_id = matches[i].object_id;
        const char *const obj_name =
            matches[i].matched_name != nullptr ? matches[i].matched_name : args;
        Inv_AddItemNTimes(obj_id, num);
        Console_Log(GS(OSD_GIVE_ITEM), obj_name);
        found = true;
    }
    Memory_FreePointer(&matches);

    if (!found) {
        Console_LogError(GS(OSD_INVALID_ITEM), args);
        return CR_FAILURE;
    }

    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("give", M_Entrypoint, GS_ID(CONSOLE_HELP_GIVE))

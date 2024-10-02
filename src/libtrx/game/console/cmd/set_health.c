
#include "game/console/cmd/pos.h"
#include "game/console/common.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/objects/common.h"
#include "strings.h"
#include "utils.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    ITEM *const lara_item = Lara_GetItem();
    if (String_IsEmpty(ctx->args)) {
        Console_Log(GS(OSD_CURRENT_HEALTH_GET), lara_item->hit_points);
        return CR_SUCCESS;
    }

    int32_t hp;
    if (!String_ParseInteger(ctx->args, &hp)) {
        return CR_BAD_INVOCATION;
    }
    CLAMP(hp, 0, LARA_MAX_HITPOINTS);

    lara_item->hit_points = hp;
    Console_Log(GS(OSD_CURRENT_HEALTH_SET), hp);
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_SetHealth = {
    .prefix = "hp",
    .proc = M_Entrypoint,
};

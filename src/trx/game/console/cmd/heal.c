#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game.h>
#include <trx/game/game_string.h>
#include <trx/game/lara/common.h>
#include <trx/game/lara/const.h>
#include <trx/game/lara/misc.h>
#include <trx/strings.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    ITEM *const lara_item = Lara_GetItem();
    if (lara_item->hit_points == LARA_MAX_HITPOINTS) {
        Console_LogWarning(GS(OSD_HEAL_ALREADY_FULL_HP));
        return CR_SUCCESS;
    }

    lara_item->hit_points = LARA_MAX_HITPOINTS;
    Lara_Extinguish();
    Console_Log(GS(OSD_HEAL_SUCCESS));
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("heal", M_Entrypoint, GS_ID(CONSOLE_HELP_HEAL))

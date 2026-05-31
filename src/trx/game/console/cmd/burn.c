#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lara.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_CHEAT) {
        return CR_UNAVAILABLE;
    }

    bool new_state = lara->burn;
    if (String_IsEmpty(ctx->args)) {
        new_state = !new_state;
    } else if (!String_ParseBool(ctx->args, &new_state)) {
        return CR_BAD_INVOCATION;
    }

    if (lara->burn == new_state) {
        Console_LogWarning(
            new_state ? GS("general/osd/burn_fail_already_on")
                      : GS("general/osd/burn_fail_already_off"));
        return CR_SUCCESS;
    }

    if (new_state) {
        Lara_CatchFire();
        Console_Log(GS("general/osd/burn_on"));
    } else {
        Lara_Extinguish();
        Console_Log(GS("general/osd/burn_off"));
    }
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("burn", M_Entrypoint, GS_ID("console/cmd/burn/help"))

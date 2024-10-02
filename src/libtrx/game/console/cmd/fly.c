#include "game/console/cmd/fly.h"

#include "game/game.h"
#include "game/game_string.h"
#include "game/lara/cheat.h"
#include "game/lara/common.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    bool enable;
    if (String_ParseBool(ctx->args, &enable)) {
        if (enable) {
            Lara_Cheat_EnterFlyMode();
        } else {
            Lara_Cheat_ExitFlyMode();
        }
        return CR_SUCCESS;
    }

    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_CHEAT) {
        Lara_Cheat_ExitFlyMode();
    } else {
        Lara_Cheat_EnterFlyMode();
    }
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_Fly = {
    .prefix = "fly",
    .proc = M_Entrypoint,
};

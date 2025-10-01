#include "config.h"
#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game.h"
#include "game/game_string.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    bool enable;
    if (String_ParseBool(ctx->args, &enable)) {
        g_Config.debug.enable_endless_sprint = enable;
        Config_Update();
    } else if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    } else {
        g_Config.debug.enable_endless_sprint =
            !g_Config.debug.enable_endless_sprint;
        Config_Update();
    }

    Console_Log(
        g_Config.debug.enable_endless_sprint ? GS(CMD_INF_SPRINT_SWITCHED_ON)
                                             : GS(CMD_INF_SPRINT_SWITCHED_OFF));
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND(
    "restless", M_Entrypoint, GS_ID(CONSOLE_HELP_INF_SPRINT))

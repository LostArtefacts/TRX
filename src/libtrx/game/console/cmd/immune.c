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
        g_Config.debug.enable_invulnerability = enable;
        Config_Update();
    } else if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    } else {
        g_Config.debug.enable_invulnerability =
            !g_Config.debug.enable_invulnerability;
        Config_Update();
    }

    Console_Log(
        g_Config.debug.enable_invulnerability ? GS(CMD_IMMUNE_SWITCHED_ON)
                                              : GS(CMD_IMMUNE_SWITCHED_OFF));
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("immune", M_Entrypoint, GS_ID(CONSOLE_HELP_IMMUNE))
REGISTER_CONSOLE_COMMAND("immunity", M_Entrypoint, GS_ID(CONSOLE_HELP_IMMUNE))

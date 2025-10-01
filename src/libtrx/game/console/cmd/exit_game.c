#include "game/console/registry.h"
#include "game/game_flow/common.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    GF_OverrideCommand((GF_COMMAND) { .action = GF_EXIT_GAME });
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("exit", M_Entrypoint, GS_ID(CONSOLE_HELP_EXIT))
REGISTER_CONSOLE_COMMAND("quit", M_Entrypoint, GS_ID(CONSOLE_HELP_EXIT))

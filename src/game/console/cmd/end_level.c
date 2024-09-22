#include "game/console/cmd/end_level.h"

#include "game/lara/cheat.h"

#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_Equivalent(ctx->args, "")) {
        return CR_BAD_INVOCATION;
    }

    Lara_Cheat_EndLevel();
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_EndLevel = {
    .prefix = "endlevel",
    .proc = M_Entrypoint,
};

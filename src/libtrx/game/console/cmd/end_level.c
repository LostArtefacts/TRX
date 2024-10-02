#include "game/console/cmd/end_level.h"

#include "game/game.h"
#include "game/lara/cheat.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    if (Game_GetCurrentLevelType() == GFL_TITLE) {
        return CR_UNAVAILABLE;
    }

    Lara_Cheat_EndLevel();
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_EndLevel = {
    .prefix = "end-?level|next-?level",
    .proc = M_Entrypoint,
};

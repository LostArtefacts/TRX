#include "game/console/cmd/speed.h"

#include "game/clock.h"
#include "game/game_string.h"

#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_Equivalent(ctx->args, "")) {
        Console_Log(GS(OSD_SPEED_GET), Clock_GetTurboSpeed());
        return CR_SUCCESS;
    }

    int32_t num = -1;
    if (String_ParseInteger(ctx->args, &num)) {
        Clock_SetTurboSpeed(num);
        return CR_SUCCESS;
    }

    return CR_BAD_INVOCATION;
}

CONSOLE_COMMAND g_Console_Cmd_Speed = {
    .prefix = "speed",
    .proc = M_Entrypoint,
};

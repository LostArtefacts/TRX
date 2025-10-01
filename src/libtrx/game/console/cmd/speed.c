#include "game/clock.h"
#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "strings.h"

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

REGISTER_CONSOLE_COMMAND("speed", M_Entrypoint, GS_ID(CONSOLE_HELP_SPEED))

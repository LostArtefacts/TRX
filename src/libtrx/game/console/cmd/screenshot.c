#include "config.h"
#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/output.h"
#include "screenshot.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx)
{
    const char *const arg = ctx->args;
    if (arg == nullptr || String_IsEmpty(arg)) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    } else {
        Screenshot_MakeToPath(arg);
    }
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND(
    "screenshot", M_Entrypoint, GS_ID(CONSOLE_HELP_SCREENSHOT))

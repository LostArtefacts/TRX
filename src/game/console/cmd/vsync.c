#include "game/console/cmd/vsync.h"

#include "config.h"

#include <libtrx/game/console/cmd/config.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(
            &g_Config.rendering.enable_vsync),
        ctx->args);
}

CONSOLE_COMMAND g_Console_Cmd_VSync = {
    .prefix = "vsync",
    .proc = M_Entrypoint,
};

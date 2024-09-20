#include "game/console/cmd/wireframe.h"

#include "config.h"

#include <libtrx/game/console/cmd/config.h>

static COMMAND_RESULT M_Entrypoint(const char *const args);

static COMMAND_RESULT M_Entrypoint(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(
            &g_Config.rendering.enable_wireframe),
        args);
}

CONSOLE_COMMAND g_Console_Cmd_Wireframe = {
    .prefix = "wireframe",
    .proc = M_Entrypoint,
};

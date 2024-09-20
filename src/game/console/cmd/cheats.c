#include "game/console/cmd/cheats.h"

#include "config.h"

#include <libtrx/game/console/cmd/config.h>

static COMMAND_RESULT M_Entrypoint(const char *const args);

static COMMAND_RESULT M_Entrypoint(const char *const args)
{
    return Console_Cmd_Config_Helper(
        Console_Cmd_Config_GetOptionFromTarget(&g_Config.enable_cheats), args);
}

CONSOLE_COMMAND g_Console_Cmd_Cheats = {
    .prefix = "cheats",
    .proc = M_Entrypoint,
};

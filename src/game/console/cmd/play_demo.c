#include "game/console/cmd/play_demo.h"

#include "global/vars.h"

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *args)
{
    g_GameInfo.override_gf_command =
        (GAMEFLOW_COMMAND) { .action = GF_START_DEMO };
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_PlayDemo = {
    .prefix = "demo",
    .proc = M_Entrypoint,
};

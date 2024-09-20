#include "game/console/cmd/exit_to_title.h"

#include "global/vars.h"

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *args)
{
    g_GameInfo.override_gf_command =
        (GAMEFLOW_COMMAND) { .action = GF_EXIT_TO_TITLE };
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_ExitToTitle = {
    .prefix = "title",
    .proc = M_Entrypoint,
};

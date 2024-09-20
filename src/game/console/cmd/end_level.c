#include "game/console/cmd/end_level.h"

#include "game/lara/lara_cheat.h"

#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *const args)
{
    if (!String_Equivalent(args, "")) {
        return CR_BAD_INVOCATION;
    }

    Lara_Cheat_EndLevel();
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_EndLevel = {
    .prefix = "endlevel",
    .proc = M_Entrypoint,
};

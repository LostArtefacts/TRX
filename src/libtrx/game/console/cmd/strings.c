#include "config.h"
#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/game_string_manager.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    const bool success = GameStringManager_ReloadLanguage(g_Config.language);
    Console_Log(success ? GS(OSD_STRINGS_RELOADED) : GS(OSD_STRINGS_FAILED));
    return success ? CR_SUCCESS : CR_FAILURE;
}

REGISTER_CONSOLE_COMMAND("strings", M_Entrypoint, GS_ID(CONSOLE_HELP_STRINGS))

#include <trx/config.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_string.h>
#include <trx/game/game_string_manager.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    const bool success = GameStringManager_ReloadLanguage(g_Config.language);
    if (success) {
        Console_Log("%s", GS(OSD_STRINGS_RELOADED));
    } else {
        Console_LogError("%s", GS(OSD_STRINGS_FAILED));
    }
    return success ? CR_SUCCESS : CR_FAILURE;
}

REGISTER_CONSOLE_COMMAND("strings", M_Entrypoint, GS_ID(CONSOLE_HELP_STRINGS))

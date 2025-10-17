#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_flow/common.h"
#include "game/game_string.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_IsEmpty(ctx->args)) {
        const GF_LEVEL *const level = GF_GetGymLevel();
        if (level == nullptr) {
            Console_LogError(GS(OSD_INVALID_LEVEL));
            return CR_FAILURE;
        }
        GF_OverrideCommand((GF_COMMAND) {
            .action = GF_SELECT_GAME,
            .param = level->num,
        });
        Console_Log(GS(OSD_PLAY_LEVEL), level->title);
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("gym", M_Entrypoint, GS_ID(CONSOLE_HELP_PLAY_GYM))
REGISTER_CONSOLE_COMMAND("home", M_Entrypoint, GS_ID(CONSOLE_HELP_PLAY_GYM))

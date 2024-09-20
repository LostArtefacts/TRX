#include "game/console/cmd/play_level.h"

#include "game/game_string.h"
#include "game/gameflow.h"
#include "global/vars.h"

#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *const args)
{
    if (String_Equivalent(args, "")) {
        return CR_BAD_INVOCATION;
    }

    int32_t level_to_load = -1;
    if (!String_ParseInteger(args, &level_to_load)) {
        for (int i = 0; i < g_GameFlow.level_count; i++) {
            if (String_CaseSubstring(g_GameFlow.levels[i].level_title, args)
                != NULL) {
                level_to_load = i;
                break;
            }
        }
        if (level_to_load == -1 && String_Equivalent(args, "gym")) {
            level_to_load = g_GameFlow.gym_level_num;
        }
        if (level_to_load == -1) {
            Console_Log(GS(OSD_INVALID_LEVEL));
            return CR_FAILURE;
        }
    }

    if (level_to_load == -1) {
        return CR_BAD_INVOCATION;
    }

    if (level_to_load >= g_GameFlow.level_count) {
        Console_Log(GS(OSD_INVALID_LEVEL));
        return CR_FAILURE;
    }

    g_GameInfo.override_gf_command = (GAMEFLOW_COMMAND) {
        .action = GF_SELECT_GAME,
        .param = level_to_load,
    };
    Console_Log(
        GS(OSD_PLAY_LEVEL), g_GameFlow.levels[level_to_load].level_title);
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_PlayLevel = {
    .prefix = "play|level",
    .proc = M_Entrypoint,
};

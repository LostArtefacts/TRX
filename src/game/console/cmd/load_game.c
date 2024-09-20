#include "game/console/cmd/load_game.h"

#include "config.h"
#include "game/game_string.h"
#include "global/vars.h"

#include <libtrx/strings.h>

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *args)
{
    int32_t slot_num;
    if (!String_ParseInteger(args, &slot_num)) {
        return CR_BAD_INVOCATION;
    }

    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= g_Config.maximum_save_slots) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_INVALID_SLOT), slot_num);
        return CR_FAILURE;
    }

    if (g_SavegameRequester.items[slot_idx].is_blocked) {
        Console_Log(GS(OSD_LOAD_GAME_FAIL_UNAVAILABLE_SLOT), slot_num);
        return CR_FAILURE;
    }

    g_GameInfo.override_gf_command = (GAMEFLOW_COMMAND) {
        .action = GF_START_SAVED_GAME,
        .param = slot_idx,
    };
    Console_Log(GS(OSD_LOAD_GAME), slot_num);
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_LoadGame = {
    .prefix = "load",
    .proc = M_Entrypoint,
};

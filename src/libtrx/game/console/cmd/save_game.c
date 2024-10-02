#include "game/console/cmd/save_game.h"

#include "config.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/gameflow/common.h"
#include "game/savegame.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    int32_t slot_num;
    if (!String_ParseInteger(ctx->args, &slot_num)) {
        return CR_BAD_INVOCATION;
    }
    const int32_t slot_idx = slot_num - 1; // convert 1-indexing to 0-indexing

    if (slot_idx < 0 || slot_idx >= Savegame_GetSlotCount()) {
        Console_Log(GS(OSD_SAVE_GAME_FAIL_INVALID_SLOT), slot_num);
        return CR_BAD_INVOCATION;
    }

    Savegame_Save(slot_idx);
    Console_Log(GS(OSD_SAVE_GAME), slot_num);
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_SaveGame = {
    .prefix = "save",
    .proc = M_Entrypoint,
};

#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/rooms.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (GF_GetCurrentLevel() == nullptr
        || GF_GetCurrentLevel()->type == GFL_TITLE) {
        return CR_UNAVAILABLE;
    }

    if (!Game_IsLoaded()) {
        return CR_UNAVAILABLE;
    }

    bool new_state = Room_GetFlipStatus();
    if (String_IsEmpty(ctx->args)) {
        new_state = !new_state;
    } else if (!String_ParseBool(ctx->args, &new_state)) {
        return CR_BAD_INVOCATION;
    }

    if (Room_GetFlipStatus() == new_state) {
        Console_LogWarning(
            new_state ? GS(OSD_FLIPMAP_FAIL_ALREADY_ON)
                      : GS(OSD_FLIPMAP_FAIL_ALREADY_OFF));
        return CR_SUCCESS;
    }

    Room_FlipMap();
    Console_Log(new_state ? GS(OSD_FLIPMAP_ON) : GS(OSD_FLIPMAP_OFF));
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND("flip", M_Entrypoint, GS_ID(CONSOLE_HELP_FLIPMAP))
REGISTER_CONSOLE_COMMAND("flipmap", M_Entrypoint, GS_ID(CONSOLE_HELP_FLIPMAP))

#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "game/music.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    int32_t track_to_play = -1;
    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    } else if (String_ParseInteger(ctx->args, &track_to_play)) {
        if (Music_Play_Direct(track_to_play, MPM_ALWAYS)) {
            Console_Log(GS(OSD_PLAY_MUSIC_TRACK), track_to_play);
        } else {
            Console_LogError(GS(OSD_INVALID_MUSIC_TRACK));
        }
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("music", M_Entrypoint, GS_ID(CONSOLE_HELP_MUSIC))

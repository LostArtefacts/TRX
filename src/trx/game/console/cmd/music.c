#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/music.h>
#include <trx/strings.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    int32_t track_to_play = -1;
    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    } else if (String_ParseInteger(ctx->args, &track_to_play)) {
        if (track_to_play == 0 || track_to_play == -1) {
            Music_Stop();
            Console_Log(GS(CMD_PLAY_MUSIC_STOPPED));
        } else if (Music_Play_Direct(track_to_play, MPM_ONCE)) {
            Console_Log(GS(CMD_PLAY_MUSIC_TRACK), track_to_play);
        } else {
            Console_LogError(GS(CMD_INVALID_MUSIC_TRACK));
        }
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("music", M_Entrypoint, GS_ID(CONSOLE_HELP_MUSIC))

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/music.h>

static bool M_IsPlayableTrack(const int32_t value, void *)
{
    return Music_IsTrackAvailable_Direct((MUSIC_ID)value);
}

static void M_LogAvailableTracks(void)
{
    const int32_t track_limit = Music_GetTrackLimit();
    if (track_limit <= 0) {
        return;
    }

    char *ranges =
        String_FormatRanges(0, track_limit - 1, M_IsPlayableTrack, nullptr);
    Console_Log(GS("console/cmd/music/available_tracks"), ranges);
    Memory_FreePointer(&ranges);
}

static void M_ShowStatus(void)
{
    const MUSIC_ID current_track = Music_GetCurrentPlayingTrack();
    const MUSIC_ID looped_track = Music_GetCurrentLoopedTrack();
    if (current_track == MX_INACTIVE) {
        Console_Log("%s", GS("console/cmd/music/current_none"));
    } else {
        Console_Log(GS("console/cmd/music/current"), current_track);
    }

    if (current_track != MX_INACTIVE && looped_track != MX_INACTIVE
        && current_track != looped_track) {
        Console_Log(GS("console/cmd/music/deferred_ambient"), looped_track);
    }

    const int32_t stream_count = Music_GetStreamCount();
    const char *overlay_tracks = nullptr;
    int32_t overlay_count = 0;
    for (int32_t i = 0; i < stream_count; i++) {
        MUSIC_STREAM_STATE state = {};
        if (!Music_GetStreamState(i, &state) || state.mode != MPM_OVERLAY) {
            continue;
        }

        overlay_tracks = overlay_tracks == nullptr
            ? String_FormatStatic("%d", state.track_id)
            : String_FormatStatic("%s, %d", overlay_tracks, state.track_id);
        overlay_count++;
    }

    if (overlay_count > 0) {
        Console_Log(GS("console/cmd/music/overlay"), overlay_tracks);
    }
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    int32_t track_to_play = -1;
    if (String_IsEmpty(ctx->args)) {
        M_ShowStatus();
        return CR_SUCCESS;
    } else if (String_Equivalent(ctx->args, "stop")) {
        Music_Stop();
        Console_Log(GS("console/cmd/play_music/stopped"));
        return CR_SUCCESS;
    } else if (String_ParseInteger(ctx->args, &track_to_play)) {
        if (track_to_play == 0 || track_to_play == -1) {
            Music_Stop();
            Console_Log(GS("console/cmd/play_music/stopped"));
        } else if (!M_IsPlayableTrack(track_to_play, nullptr)) {
            Console_LogError(GS("console/cmd/play_music/invalid_track"));
            M_LogAvailableTracks();
        } else if (Music_Play_Direct(track_to_play, MPM_ONCE)) {
            Console_Log(GS("console/cmd/play_music/track"), track_to_play);
        } else {
            Console_LogError(GS("console/cmd/play_music/invalid_track"));
            M_LogAvailableTracks();
        }
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("music", M_Entrypoint, GS_ID("console/cmd/music/help"))

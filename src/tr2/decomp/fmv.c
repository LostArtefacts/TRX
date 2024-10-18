#include "decomp/fmv.h"

#include "decomp/decomp.h"
#include "game/input.h"
#include "game/music.h"
#include "global/funcs.h"
#include "global/vars.h"

static void M_Begin(void);
static void M_Play(const char *file_name);
static void M_End(void);

static void M_Begin(void)
{
    g_IsFMVPlaying = true;
    Music_Stop();
    ShowCursor(false);
    RenderFinish(true);
}

static void M_Play(const char *const file_name)
{
    const char *const full_path = GetFullPath(file_name);
    WinPlayFMV(full_path, true);
    WinStopFMV(true);
}

static void M_End(void)
{
    g_IsFMVPlaying = false;
    if (!g_IsGameToExit) {
        FmvBackToGame();
    }
    ShowCursor(true);
}

bool __cdecl PlayFMV(const char *const file_name)
{
    M_Begin();
    M_Play(file_name);
    M_End();
    return g_IsGameToExit;
}

bool __cdecl IntroFMV(
    const char *const file_name_1, const char *const file_name_2)
{
    M_Begin();
    M_Play(file_name_1);
    M_Play(file_name_2);
    M_End();
    return g_IsGameToExit;
}

void __cdecl FmvBackToGame(void)
{
    RenderStart(true);
}

void __cdecl WinPlayFMV(const char *const file_name, const bool is_playback)
{
    const RECT rect = {
        .left = 0,
        .top = 0,
        .right = 640,
        .bottom = 480,
    };
    if (Player_PassInDirectDrawObject(g_DDraw) != 0) {
        return;
    }

    if (Player_InitMovie(&g_MovieContext, 0, 0, file_name, 0x200000) != 0) {
        return;
    }

    if (Movie_GetFormat(g_MovieContext) != 130) {
        return;
    }

    const int32_t y_size = Movie_GetYSize(g_MovieContext);
    const int32_t x_size = Movie_GetXSize(g_MovieContext);
    const int32_t x_offset = 320 - x_size;
    const int32_t y_offset = 240 - y_size;

    if (Player_InitVideo(
            &g_FmvContext, g_MovieContext, x_size, y_size, x_offset, y_offset,
            0, 0, 640, 480, 0, 1, 13)
        != 0) {
        return;
    }

    if (!is_playback) {
        return;
    }

    if (Player_InitPlaybackMode(g_GameWindowHandle, g_FmvContext, 1, 0) != 0) {
        return;
    }

    Player_BlankScreen(rect.left, rect.top, rect.right, rect.bottom);
    if (Player_InitSoundSystem(g_GameWindowHandle) != 0) {
        return;
    }

    if (Player_GetDSErrorCode() < 0) {
        return;
    }

    const int32_t sound_precision = Movie_GetSoundPrecision(g_MovieContext);
    const int32_t sound_rate = Movie_GetSoundRate(g_MovieContext);
    const int32_t sound_channels = Movie_GetSoundChannels(g_MovieContext);
    const bool is_uncompressed = sound_precision != 4;
    const int32_t sound_format = is_uncompressed ? 1 : 4;
    if (Player_InitSound(
            &g_FmvSoundContext, 16384, sound_format, is_uncompressed, 4096,
            sound_channels, sound_rate, sound_precision, 2)
        != 0) {
        return;
    }

    Movie_SetSyncAdjust(g_MovieContext, g_FmvSoundContext, 4);
    if (Player_InitMoviePlayback(
            g_MovieContext, g_FmvContext, g_FmvSoundContext)
        != 0) {
        return;
    }

    Input_Update();
    Player_StartTimer(g_MovieContext);
    Player_BlankScreen(rect.left, rect.top, rect.right, rect.bottom);
    Input_Update();
    while (Movie_GetCurrentFrame(g_MovieContext)
           < Movie_GetTotalFrames(g_MovieContext)) {
        if (Player_PlayFrame(
                g_MovieContext, g_FmvContext, g_FmvSoundContext, 0, &rect, 0, 0,
                0)
            != 0) {
            break;
        }

        if (Input_Update()) {
            break;
        }
        if ((g_Input & IN_OPTION) != 0) {
            break;
        }
    }
}

void __cdecl WinStopFMV(const bool is_playback)
{
    Player_StopTimer(g_MovieContext);
    Player_ShutDownSound(&g_FmvSoundContext);
    Player_ShutDownVideo(&g_FmvContext);
    Player_ShutDownMovie(&g_MovieContext);
    Player_ShutDownSoundSystem();
    if (is_playback) {
        Player_ReturnPlaybackMode(is_playback);
    }
}

bool __cdecl S_PlayFMV(const char *const file_name)
{
    return PlayFMV(file_name);
}

bool __cdecl S_IntroFMV(
    const char *const file_name_1, const char *const file_name_2)
{
    return IntroFMV(file_name_1, file_name_2);
}

#include "specific/s_fmv.h"

#include "config.h"
#include "game/clock.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "game/screen.h"
#include "global/lib.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "specific/s_hwr.h"
#include "specific/s_shell.h"

#include <stdbool.h>

const char *m_FMVPaths[] = {
    "fmv\\cafe.rpl",    "fmv\\mansion.rpl", "fmv\\snow.rpl",
    "fmv\\lift.rpl",    "fmv\\vision.rpl",  "fmv\\canyon.rpl",
    "fmv\\pyramid.rpl", "fmv\\prison.rpl",  "fmv\\end.rpl",
    "fmv\\core.rpl",    "fmv\\escape.rpl",  NULL,
};

void S_FMV_Init()
{
    if (Player_PassInDirectDrawObject(DDraw)) {
        S_Shell_ExitSystem("ERROR: Cannot initialise FMV player videosystem");
    }
    if (Player_InitSoundSystem(TombHWND)) {
        Player_GetDSErrorCode();
        S_Shell_ExitSystem("ERROR: Cannot prepare FMV player soundsystem");
    }
}

void S_FMV_Play(int32_t sequence)
{
    if (T1MConfig.disable_fmv) {
        return;
    }

    GameBuf_Shutdown();

    Screen_SetResolution(2);
    HWR_PrepareFMV();

    void *movie_context = NULL;
    void *fmv_context = NULL;
    void *sound_context = NULL;

    HWR_FMVInit();

    if (Player_InitMovie(
            &movie_context, 0, 0, m_FMVPaths[sequence], 0x100000)) {
        LOG_ERROR("cannot load video");
        goto cleanup;
    }

    int32_t width = Movie_GetXSize(movie_context);
    int32_t height = Movie_GetYSize(movie_context);
    int32_t x = 0;
    int32_t y = 0;
    int32_t flags = 13;
    y = (480 - 2 * height) / 2;

    if (Player_InitVideo(
            &fmv_context, movie_context, width, height, x, y, 0, 0, 640, 480, 0,
            1, flags)) {
        LOG_ERROR("cannot init video");
        goto cleanup;
    }

    if (Player_InitPlaybackMode(TombHWND, fmv_context, 1, 0)) { //
        LOG_ERROR("cannot init playback mode");
        goto cleanup;
    }

    if (SoundIsActive) {
        int32_t precision = Movie_GetSoundPrecision(movie_context);
        int32_t rate = Movie_GetSoundRate(movie_context);
        int32_t channels = Movie_GetSoundChannels(movie_context);
        if (Player_InitSound(
                &sound_context, 44100, 1, 1, 22050, channels, rate, precision,
                2)) {
            LOG_ERROR("cannot init sound");
            goto cleanup;
        }
    }

    if (Player_InitMoviePlayback(movie_context, fmv_context, sound_context)) {
        LOG_ERROR("cannot init movie playback");
        goto cleanup;
    }

    if (Player_MapVideo(fmv_context, 0)) {
        LOG_ERROR("cannot map video");
        goto cleanup;
    }

    bool keypress = false;
    int32_t total_frames = Movie_GetTotalFrames(movie_context);
    if (Player_StartTimer(movie_context)) {
        LOG_ERROR("cannot start timer");
        goto cleanup;
    }

    while (Movie_GetCurrentFrame(movie_context) < total_frames - 2) {
        if (Player_PlayFrame(
                movie_context, fmv_context, sound_context, 0, 0, 0, 0, 0)) {
            break;
        }
        Input_Update();
        S_Shell_SpinMessageLoop();
        Clock_Sync();

        if (g_Input.deselect) {
            keypress = true;
        } else if (keypress && !g_Input.deselect) {
            break;
        }
    }

    Player_ShutDownSound(&sound_context);
    Player_ShutDownVideo(&fmv_context);
    Player_ShutDownMovie(&movie_context);

cleanup:
    Player_ReturnPlaybackMode();

    GameBuf_Init();

    HWR_FMVDone();
    Screen_RestoreResolution();
}

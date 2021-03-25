#include "specific/frontend.h"

#include "config.h"
#include "global/const.h"
#include "global/lib.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "util.h"

#include <stdlib.h>

const char *FMVPaths[] = {
    "fmv\\cafe.rpl",    "fmv\\mansion.rpl", "fmv\\snow.rpl",
    "fmv\\lift.rpl",    "fmv\\vision.rpl",  "fmv\\canyon.rpl",
    "fmv\\pyramid.rpl", "fmv\\prison.rpl",  "fmv\\end.rpl",
    "fmv\\core.rpl",    "fmv\\escape.rpl",  NULL,
};

void S_Wait(int32_t nframes)
{
    for (int i = 0; i < nframes; i++) {
        S_UpdateInput();
        if (KeyData->keys_held) {
            break;
        }
        while (!WinVidSpinMessageLoop())
            ;
    }
    while (Input) {
        S_UpdateInput();
        WinVidSpinMessageLoop();
    }
}

int32_t WinPlayFMV(int32_t sequence, int32_t mode)
{
    if (T1MConfig.disable_fmv) {
        return -1;
    }

    int32_t result = 0;
    void *movie_context = NULL;
    void *fmv_context = NULL;
    void *sound_context = NULL;

    sub_40837F();
    const char *path = GetFullPath(FMVPaths[sequence]);

    if (Player_InitMovie(&movie_context, 0, 0, path, 0x100000)) {
        LOG_ERROR("cannot load video");
        goto cleanup;
    }

    int32_t width = Movie_GetXSize(movie_context);
    int32_t height = Movie_GetYSize(movie_context);
    int32_t x = 0;
    int32_t y = 0;
    int32_t tmp = 0;
    if (mode) {
        y = (480 - 2 * height) / 2;
        tmp = 13;
    } else {
        y = 0;
        tmp = 5;
    }

    if (Player_InitVideo(
            &fmv_context, movie_context, width, height, x, y, 0, 0, 640, 480, 0,
            1, tmp)) {
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

    result = 1;
    int8_t keypress = 0;
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
        sub_43D940();

        if (T1MConfig.fix_fmv_esc_key) {
            if (KeyData->keymap[1]) {
                keypress = 1;
            } else if (keypress && !KeyData->keymap[1]) {
                break;
            }
        } else {
            if (KeyData->keymap[1]) {
                break;
            }
        }
    }

    Player_ShutDownSound(&sound_context);
    Player_ShutDownVideo(&fmv_context);
    Player_ShutDownMovie(&movie_context);

cleanup:
    Player_ReturnPlaybackMode();
    return result;
}

int32_t S_PlayFMV(int32_t sequence, int32_t mode)
{
    if (GameMemoryPointer) {
        free(GameMemoryPointer);
    }

    TempVideoAdjust(2, 1.0);
    HardwarePrepareFMV();

    int32_t ret = WinPlayFMV(sequence, mode);

    GameMemoryPointer = malloc(MALLOC_SIZE);
    if (!GameMemoryPointer) {
        S_ExitSystem("ERROR: Could not allocate enough memory");
        return -1;
    }
    init_game_malloc();

    if (IsHardwareRenderer) {
        HardwareFMVDone();
    }
    TempVideoRemove();

    return ret;
}

void T1MInjectSpecificFrontend()
{
    INJECT(0x0041CD50, S_Wait);
    INJECT(0x0041CDF0, WinPlayFMV);
    INJECT(0x0041D040, S_PlayFMV);
}

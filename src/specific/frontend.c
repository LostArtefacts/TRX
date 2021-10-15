#include "specific/frontend.h"

#include "config.h"
#include "global/const.h"
#include "global/lib.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/clock.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/hwr.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/smain.h"
#include "util.h"

#include <dinput.h>
#include <stdlib.h>

const char *FMVPaths[] = {
    "fmv\\cafe.rpl",    "fmv\\mansion.rpl", "fmv\\snow.rpl",
    "fmv\\lift.rpl",    "fmv\\vision.rpl",  "fmv\\canyon.rpl",
    "fmv\\pyramid.rpl", "fmv\\prison.rpl",  "fmv\\end.rpl",
    "fmv\\core.rpl",    "fmv\\escape.rpl",  NULL,
};

SG_COL S_Colour(int32_t red, int32_t green, int32_t blue)
{
    int32_t best_dist = SQUARE(256) * 3;
    SG_COL best_entry = 0;
    for (int i = 0; i < 256; i++) {
        RGB888 *col = &GamePalette[i];
        int32_t dr = red - col->r;
        int32_t dg = green - col->g;
        int32_t db = blue - col->b;
        int32_t dist = SQUARE(dr) + SQUARE(dg) + SQUARE(db);
        if (dist < best_dist) {
            best_dist = dist;
            best_entry = i;
        }
    }
    return best_entry;
}

RGB888 S_ColourFromPalette(int8_t idx)
{
    RGB888 ret;
    ret.r = 4 * GamePalette[idx].r;
    ret.g = 4 * GamePalette[idx].g;
    ret.b = 4 * GamePalette[idx].b;
    return ret;
}

void S_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 color)
{
    HWR_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void S_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br)
{
    HWR_Draw2DQuad(sx, sy, sx + w, sy + h, tl, tr, bl, br);
}

void S_DrawScreenLine(int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 col)
{
    HWR_Draw2DLine(sx, sy, sx + w, sy + h, col, col);
}

void S_DrawScreenBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    RGB888 rgb_border_light = S_ColourFromPalette(15);
    RGB888 rgb_border_dark = S_ColourFromPalette(31);
    S_DrawScreenLine(sx - 1, sy - 1, w + 3, 0, rgb_border_light);
    S_DrawScreenLine(sx, sy, w + 1, 0, rgb_border_dark);
    S_DrawScreenLine(w + sx + 1, sy, 0, h + 1, rgb_border_light);
    S_DrawScreenLine(w + sx + 2, sy - 1, 0, h + 3, rgb_border_dark);
    S_DrawScreenLine(w + sx + 1, h + sy + 1, -w - 1, 0, rgb_border_light);
    S_DrawScreenLine(w + sx + 2, h + sy + 2, -w - 3, 0, rgb_border_dark);
    S_DrawScreenLine(sx - 1, h + sy + 2, 0, -3 - h, rgb_border_light);
    S_DrawScreenLine(sx, h + sy + 1, 0, -1 - h, rgb_border_dark);
}

void S_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    HWR_DrawTranslucentQuad(sx, sy, sx + w, sy + h);
}

void S_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags)
{
    PHD_SPRITE *sprite = &PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * (sprite->x1 >> 3) / PHD_ONE);
    int32_t x2 = sx + (scale_h * (sprite->x2 >> 3) / PHD_ONE);
    int32_t y1 = sy + (scale_v * (sprite->y1 >> 3) / PHD_ONE);
    int32_t y2 = sy + (scale_v * (sprite->y2 >> 3) / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < PhdWinWidth && y1 < PhdWinHeight) {
        HWR_DrawSprite(x1, y1, x2, y2, 8 * z, sprnum, shade);
    }
}

void S_DrawScreenSprite2d(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page)
{
    PHD_SPRITE *sprite = &PhdSpriteInfo[(signed __int16)sprnum];
    int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    int32_t x2 = sx + (scale_h * sprite->x2 / PHD_ONE);
    int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    int32_t y2 = sy + (scale_v * sprite->y2 / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < PhdWinWidth && y1 < PhdWinHeight) {
        HWR_DrawSprite(x1, y1, x2, y2, 200, sprnum, 0);
    }
}

void S_FinishInventory()
{
    if (InvMode != INV_TITLE_MODE) {
        TempVideoRemove();
    }
    ModeLock = 0;
    if (RenderSettings != OldRenderSettings) {
        HWR_DownloadTextures(-1);
        OldRenderSettings = RenderSettings;
    }
}

void S_FadeToBlack()
{
    memset(GamePalette, 0, sizeof(GamePalette));
    HWR_FadeToPal(20, GamePalette);
    HWR_FadeWait();
}

void S_Wait(int32_t nticks)
{
    for (int i = 0; i < nticks; i++) {
        S_UpdateInput();
        if (Input) {
            break;
        }
        ClockSyncTicks(1);
    }
    while (Input) {
        S_UpdateInput();
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

    HWR_FMVInit();
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
        S_UpdateInput();
        WinSpinMessageLoop();
        ClockSync();

        if (Input & IN_DESELECT) {
            keypress = 1;
        } else if (keypress && !(Input & IN_DESELECT)) {
            break;
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
    HWR_PrepareFMV();

    int32_t ret = WinPlayFMV(sequence, mode);

    GameMemoryPointer = malloc(MALLOC_SIZE);
    if (!GameMemoryPointer) {
        S_ExitSystem("ERROR: Could not allocate enough memory");
        return -1;
    }
    init_game_malloc();

    HWR_FMVDone();
    TempVideoRemove();

    return ret;
}

void FMVInit()
{
    if (Player_PassInDirectDrawObject(DDraw)) {
        S_ExitSystem("ERROR: Cannot initialise FMV player videosystem");
    }
    if (Player_InitSoundSystem(TombHWND)) {
        Player_GetDSErrorCode();
        S_ExitSystem("ERROR: Cannot prepare FMV player soundsystem");
    }
}

void T1MInjectSpecificFrontend()
{
    INJECT(0x0041C0F0, S_Colour);
    INJECT(0x0041C180, S_DrawScreenSprite2d);
    INJECT(0x0041C2D0, S_DrawScreenSprite);
    INJECT(0x0041C440, S_DrawScreenLine);
    INJECT(0x0041C520, S_DrawScreenBox);
    INJECT(0x0041CBB0, S_DrawScreenFBox);
    INJECT(0x0041CCC0, S_FinishInventory);
    INJECT(0x0041CD10, S_FadeToBlack);
    INJECT(0x0041CD50, S_Wait);
    INJECT(0x0041CDA0, FMVInit);
    INJECT(0x0041CDF0, WinPlayFMV);
    INJECT(0x0041D040, S_PlayFMV);
}

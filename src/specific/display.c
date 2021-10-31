#include "specific/display.h"

#include "3dsystem/3d_gen.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/hwr.h"
#include "specific/init.h"

#include <stdlib.h>

// The screen resolution is controlled by two variables are indices
// within an array of predefined screen resolutions.
// Actual screen resolution, sometimes different from the game
// resolution (during FMVs, main menu sequence, static pictures etc.)
static int32_t HiRes = 0;
// The resolution to render the game in. This is what gets saved in
// settings and the like.
static int32_t GameHiRes = 0;

bool SetGameScreenSizeIdx(int32_t idx)
{
    if (idx >= 0 && idx < RESOLUTIONS_SIZE) {
        GameHiRes = idx;
        return true;
    }
    return false;
}

bool SetPrevGameScreenSize()
{
    if (GameHiRes - 1 >= 0) {
        GameHiRes--;
        return true;
    }
    return false;
}

bool SetNextGameScreenSize()
{
    if (GameHiRes + 1 < RESOLUTIONS_SIZE) {
        GameHiRes++;
        return true;
    }
    return false;
}

int32_t GetGameScreenSizeIdx()
{
    return GameHiRes;
}

int32_t GetScreenSizeIdx()
{
    return HiRes;
}

int32_t GetScreenWidth()
{
    return AvailableResolutions[HiRes].width;
}

int32_t GetScreenHeight()
{
    return AvailableResolutions[HiRes].height;
}

void SetupScreenSize()
{
    int32_t res_x = GetScreenWidth();
    int32_t res_y = GetScreenHeight();

    int32_t width = res_x * ScreenSizer;
    int32_t height = res_y * ScreenSizer;
    int32_t x = (res_x - width) / 2;
    int32_t y = (res_y - height) / 2;
    phd_InitWindow(
        x, y, width, height, VIEW_NEAR, VIEW_FAR, GAME_FOV, res_x, res_y);
}

void TempVideoAdjust(int32_t hi_res, double sizer)
{
    ModeLock = 1;
    if (hi_res == HiRes && sizer == ScreenSizer) {
        return;
    }

    HiRes = hi_res;
    HWR_SwitchResolution();
}

void TempVideoRemove()
{
    ModeLock = 0;
    if (GameHiRes == HiRes && GameSizer == ScreenSizer) {
        return;
    }

    HiRes = GameHiRes;
    HWR_SwitchResolution();
}

void S_NoFade()
{
    FadeValue = 0x100000;
    FadeLimit = 0x100000;
}

void S_FadeInInventory(int32_t fade)
{
    if (CurrentLevel == GF.title_level_num) {
        HWR_DownloadPicture();
    } else {
        HWR_CopyPicture();
    }
}

void S_FadeOutInventory(int32_t fade)
{
    if (fade) {
        FadeValue = 0x180000;
        FadeLimit = 0x100000;
        FadeAdder = -32768;
    }
}

void S_CopyBufferToScreen()
{
    HWR_ClearSurfaceDepth();
    HWR_RenderEnd();
    HWR_BlitSurface(Surface3, Surface2);
    HWR_RenderToggle();
}

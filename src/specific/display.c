#include "specific/display.h"

#include "3dsystem/3d_gen.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/hwr.h"
#include "specific/init.h"

#include <stdlib.h>

// The screen resolution is controlled by two variables that are indices within
// an array of predefined screen resolutions.
// Actual screen resolution, sometimes different from the game resolution
// (during FMVs, main menu sequence, static pictures etc.)
static int32_t HiRes = 0;
// The resolution to render the game in. This is what gets saved in settings
// and the like.
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

int32_t GetGameScreenWidth()
{
    return AvailableResolutions[GameHiRes].width;
}

int32_t GetGameScreenHeight()
{
    return AvailableResolutions[GameHiRes].height;
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
    int32_t width = GetScreenWidth();
    int32_t height = GetScreenHeight();
    int32_t x = (width - width) / 2;
    int32_t y = (height - height) / 2;
    phd_InitWindow(x, y, width, height, VIEW_NEAR, VIEW_FAR, GAME_FOV);
}

void TempVideoAdjust(int32_t hi_res)
{
    ModeLock = 1;
    if (hi_res == HiRes) {
        return;
    }

    HiRes = hi_res;
    HWR_SwitchResolution();
}

void TempVideoRemove()
{
    ModeLock = 0;
    if (GameHiRes == HiRes) {
        return;
    }

    HiRes = GameHiRes;
    HWR_SwitchResolution();
}

void S_NoFade()
{
    // not implemented in TombATI
}

void S_FadeInInventory(int32_t fade)
{
    if (CurrentLevel != GF.title_level_num) {
        HWR_CopyPicture();
    }
}

void S_FadeOutInventory(int32_t fade)
{
    // not implemented in TombATI
}

void S_CopyBufferToScreen()
{
    HWR_ClearSurfaceDepth();
    HWR_RenderEnd();
    HWR_BlitSurface(Surface3, Surface2);
    HWR_RenderToggle();
}

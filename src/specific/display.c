#include "specific/display.h"

#include "3dsystem/3d_gen.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/hwr.h"
#include "specific/init.h"

#include <stdlib.h>

void SetupScreenSize()
{
    int32_t width = ((double)GameVidWidth * ScreenSizer);
    int32_t height = ((double)GameVidHeight * ScreenSizer);
    int32_t x = (GameVidWidth - width) / 2;
    int32_t y = (GameVidHeight - height) / 2;
    phd_InitWindow(
        x, y, width, height, VIEW_NEAR, VIEW_FAR, GAME_FOV, GameVidWidth,
        GameVidHeight, ScrPtr);
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

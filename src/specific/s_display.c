#include "specific/s_display.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/s_hwr.h"

void S_NoFade()
{
    // not implemented in TombATI
}

void S_FadeInInventory(int32_t fade)
{
    if (g_CurrentLevel != g_GameFlow.title_level_num) {
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
    HWR_BlitSurface(g_Surface3, g_Surface2);
    HWR_RenderToggle();
}

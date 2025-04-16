#include "game/screen.h"

#include <libtrx/config.h>
#include <libtrx/game/ui2/common.h>

int32_t UI2_GetCanvasWidth(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC) * 16 / 9;
}

int32_t UI2_GetCanvasHeight(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC);
}

float UI2_ScaleX(const float x)
{
    return Screen_GetRenderScale(x * 0x10000, RSR_GENERIC) / (float)0x10000;
}

float UI2_ScaleY(const float y)
{
    return Screen_GetRenderScale(y * 0x10000, RSR_GENERIC) / (float)0x10000;
}

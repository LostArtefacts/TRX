#include "game/screen.h"

#include <libtrx/config.h>
#include <libtrx/game/ui/common.h>

int32_t UI_GetCanvasWidth(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC) * 16 / 9;
}

int32_t UI_GetCanvasHeight(void)
{
    return Screen_GetResHeightDownscaled(RSR_GENERIC);
}

float UI_ScaleX(const float x)
{
    return Screen_GetRenderScale(x * 0x10000, RSR_GENERIC) / (float)0x10000;
}

float UI_ScaleY(const float y)
{
    return Screen_GetRenderScale(y * 0x10000, RSR_GENERIC) / (float)0x10000;
}

#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/scaler.h>
#include <libtrx/game/ui/common.h>

int32_t UI_GetCanvasWidth(void)
{
    return Scaler_CalcInverse(g_PhdWinWidth, SCALER_TARGET_GENERIC);
}

int32_t UI_GetCanvasHeight(void)
{
    return Scaler_CalcInverse(g_PhdWinHeight, SCALER_TARGET_GENERIC);
}

float UI_ScaleX(const float x)
{
    return Scaler_Calc(x, SCALER_TARGET_GENERIC);
}

float UI_ScaleY(const float y)
{
    return Scaler_Calc(y, SCALER_TARGET_GENERIC);
}

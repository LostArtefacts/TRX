#include "game/scaler.h"

#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

static int32_t M_DoCalc(
    int32_t unit, int32_t base_width, int32_t base_height, double factor)
{
    const int32_t sign = unit < 0 ? -1 : 1;
    const int32_t scale_x = g_PhdWinWidth > base_width
        ? ((double)g_PhdWinWidth * ABS(unit) * factor) / MAX(1, base_width)
        : ABS(unit) * factor;
    const int32_t scale_y = g_PhdWinHeight > base_height
        ? ((double)g_PhdWinHeight * ABS(unit) * factor) / MAX(1, base_height)
        : ABS(unit) * factor;
    return MIN(scale_x, scale_y) * sign;
}

double Scaler_GetScale(const SCALER_TARGET target)
{
    switch (target) {
    case SCALER_TARGET_BAR:
        return g_Config.ui.bar_scale;
    case SCALER_TARGET_TEXT:
        return g_Config.ui.text_scale;
    case SCALER_TARGET_ASSAULT_DIGITS:
        return g_Config.ui.text_scale;
    default:
        return 1.0;
    }
}

int32_t Scaler_Calc(const int32_t unit, const SCALER_TARGET target)
{
    return M_DoCalc(unit, 640, 480, Scaler_GetScale(target));
}

int32_t Scaler_CalcInverse(const int32_t unit, const SCALER_TARGET target)
{
    return unit * 0x10000 / MAX(1, Scaler_Calc(0x10000, target));
}

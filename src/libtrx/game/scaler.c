#include "game/scaler.h"

#include "config.h"
#include "game/viewport.h"
#include "utils.h"

static int32_t M_DoCalc(
    const int32_t unit, const int32_t base_width, const int32_t base_height,
    const double factor)
{
    const int32_t vp_width = Viewport_GetWidth(VIEWPORT_GAME);
    const int32_t vp_height = Viewport_GetHeight(VIEWPORT_GAME);
    const int32_t sign = unit < 0 ? -1 : 1;
    const int32_t sx =
        ((double)vp_width * ABS(unit) * factor) / MAX(1, base_width);
    const int32_t sy =
        ((double)vp_height * ABS(unit) * factor) / MAX(1, base_height);
    return MIN(sx, sy) * sign;
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

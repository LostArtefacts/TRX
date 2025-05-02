#include "game/ui/elements/bar_lara_air.h"

#include "game/ui/elements/bar.h"

#include <libtrx/config.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/lara/const.h>

bool UI_LaraAirBar(const bool blink_state)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const bool is_blinking = lara->air <= LARA_MAX_AIR * UI_BAR_BLINK_THRESHOLD;

    bool show = lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_SURFACE;
    switch (g_Config.ui.airbar_show_mode) {
    case BSM_DEFAULT:
        break;
    case BSM_FLASHING_ONLY:
        show &= is_blinking;
        break;
    case BSM_NEVER:
        show = false;
        break;
    default:
        break;
    }
    if (!show) {
        return false;
    }

    UI_Bar((UI_BAR_SETTINGS) {
        .color = g_Config.ui.airbar_color,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = is_blinking && blink_state ? 0 : lara->air,
        .max_value = LARA_MAX_AIR,
    });
    return true;
}

#include "game/ui/elements/bar_lara_exposure.h"

#include "config.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/ui/elements/bar.h"

bool UI_LaraExposureBar(const bool blink_state)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const bool is_blinking =
        lara->exposure_timer <= LARA_MAX_EXPOSURE * UI_BAR_BLINK_THRESHOLD
        && g_Config.ui.lara_exposure_bar.show_mode != BSM_PS1;

    bool show = lara->exposure_timer < LARA_MAX_EXPOSURE;
    switch (g_Config.ui.lara_exposure_bar.show_mode) {
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

    int32_t value = is_blinking && blink_state ? 0 : lara->exposure_timer;
    CLAMPL(value, 0);
    UI_Bar((UI_BAR_SETTINGS) {
        .type = UI_BAR_LARA_EXPOSURE,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = value,
        .max_value = LARA_MAX_EXPOSURE,
    });
    return true;
}

#include "game/ui/elements/bar_lara_sprint.h"

#include "config.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/ui/elements/bar.h"

bool UI_LaraSprintBar(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    bool show = lara->sprint_timer < LARA_MAX_SPRINT;
    switch (g_Config.ui.lara_sprint_bar.show_mode) {
    case BSM_DEFAULT:
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
        .type = UI_BAR_LARA_STAMINA,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = lara->sprint_timer,
        .max_value = LARA_MAX_SPRINT,
    });
    return true;
}

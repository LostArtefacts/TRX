#include "game/ui/elements/bar_lara_hp.h"

#include "config.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/ui/elements/bar.h"

static int32_t m_OldHealth = 0;
static int32_t m_HitTimer = 0;

void UI_LaraHealthBar_Control(void)
{
    m_HitTimer--;
    CLAMPL(m_HitTimer, 0);
}

void UI_LaraHealthBar_SetTimer(const int16_t timer)
{
    m_HitTimer = timer;
}

bool UI_LaraHealthBar(const bool blink_state, const BAR_SHOW_MODE show_mode)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    int32_t health = lara_item->hit_points;
    CLAMP(health, 0, LARA_MAX_HITPOINTS);

    if (m_OldHealth != health) {
        m_OldHealth = health;
        m_HitTimer = 40;
    }

    const bool is_blinking =
        health <= LARA_MAX_HITPOINTS * UI_BAR_BLINK_THRESHOLD;
    const bool is_recently_hurt = m_HitTimer > 0;
    bool show =
        is_recently_hurt || health <= 0 || lara->gun_status == LGS_READY;
    switch (show_mode) {
    case BSM_FLASHING_OR_DEFAULT:
        show |= is_blinking;
        break;
    case BSM_FLASHING_ONLY:
        show = is_blinking;
        break;
    case BSM_ALWAYS:
        show = true;
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
        .type = UI_BAR_LARA_HP,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = is_blinking && blink_state ? 0 : health,
        .max_value = LARA_MAX_HITPOINTS,
    });
    return true;
}

#include "game/ui/elements/bar_enemy_hp.h"

#include "config.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/lara/common.h"
#include "game/lara/const.h"
#include "game/objects/vars.h"
#include "game/ui/elements/bar.h"

bool UI_EnemyHealthBar(void)
{
    const ITEM *const target = Lara_GetLaraInfo()->target;
    if (target == nullptr) {
        return false;
    }
    const OBJECT *const obj = Object_Get(target->object_id);
    const bool is_ally = Object_IsType(target->object_id, g_AllyObjects);

    bool show = false;
    switch (g_Config.ui.enemy_health_bar.show_mode) {
    case BSM_DEFAULT:
    case BSM_PS1:
    case BSM_NEVER:
        show = false;
        break;
    case BSM_FLASHING_ONLY:
    case BSM_FLASHING_OR_DEFAULT:
    case BSM_ALWAYS:
        show = true;
        break;
    case BSM_BOSS_ONLY:
        show = Object_IsType(target->object_id, g_BossObjects);
        break;
    }
    if (!show) {
        return false;
    }

    UI_Bar((UI_BAR_SETTINGS) {
        .type = is_ally ? UI_BAR_ALLY_HP : UI_BAR_ENEMY_HP,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = target->hit_points,
        .max_value =
            target->max_hit_points * (Game_IsBonusFlagSet(GBF_NGPLUS) ? 2 : 1),
    });
    return true;
}

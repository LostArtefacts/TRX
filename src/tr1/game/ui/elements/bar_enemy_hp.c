#include "game/ui/elements/bar_enemy_hp.h"

#include "game/creature.h"
#include "game/ui/elements/bar.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/lara/const.h>

bool UI_EnemyHealthBar(void)
{
    const ITEM *const target = Lara_GetLaraInfo()->target;
    if (target == nullptr) {
        return false;
    }
    const OBJECT *const obj = Object_Get(target->object_id);

    bool show = false;
    switch (g_Config.ui.enemy_healthbar_show_mode) {
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
        show = Creature_IsBoss(Item_GetIndex(target));
        break;
    }
    if (!show) {
        return false;
    }

    UI_Bar((UI_BAR_SETTINGS) {
        .color = g_Config.ui.enemy_healthbar_color,
        .w = UI_BAR_WIDTH,
        .h = UI_BAR_HEIGHT,
        .value = target->hit_points,
        .max_value =
            obj->hit_points * (Game_IsBonusFlagSet(GBF_NGPLUS) ? 2 : 1),
    });
    return true;
}

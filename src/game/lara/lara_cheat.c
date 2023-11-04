#include "game/lara/lara_cheat.h"

#include "game/effects/exploding_death.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void Lara_CheckCheatMode(void)
{
    static int32_t cheat_mode = 0;
    static int16_t cheat_angle = 0;
    static int32_t cheat_turn = 0;

    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    LARA_STATE as = g_LaraItem->current_anim_state;
    switch (cheat_mode) {
    case 0:
        if (as == LS_WALK) {
            cheat_mode = 1;
        }
        break;

    case 1:
        if (as != LS_WALK) {
            cheat_mode = as == LS_STOP ? 2 : 0;
        }
        break;

    case 2:
        if (as != LS_STOP) {
            cheat_mode = as == LS_BACK ? 3 : 0;
        }
        break;

    case 3:
        if (as != LS_BACK) {
            cheat_mode = as == LS_STOP ? 4 : 0;
        }
        break;

    case 4:
        if (as != LS_STOP) {
            cheat_angle = g_LaraItem->pos.y_rot;
        }
        cheat_turn = 0;
        if (as == LS_TURN_L) {
            cheat_mode = 5;
        } else if (as == LS_TURN_R) {
            cheat_mode = 6;
        } else {
            cheat_mode = 0;
        }
        break;

    case 5:
        if (as == LS_TURN_L || as == LS_FAST_TURN) {
            cheat_turn += (int16_t)(g_LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = g_LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn < -94208 ? 7 : 0;
        }
        break;

    case 6:
        if (as == LS_TURN_R || as == LS_FAST_TURN) {
            cheat_turn += (int16_t)(g_LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = g_LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn > 94208 ? 7 : 0;
        }
        break;

    case 7:
        if (as != LS_STOP) {
            cheat_mode = as == LS_COMPRESS ? 8 : 0;
        }
        break;

    case 8:
        if (g_LaraItem->fall_speed > 0) {
            if (as == LS_JUMP_FORWARD) {
                g_LevelComplete = true;
            } else if (as == LS_JUMP_BACK) {
                Inv_AddItem(O_SHOTGUN_ITEM);
                Inv_AddItem(O_MAGNUM_ITEM);
                Inv_AddItem(O_UZI_ITEM);
                g_Lara.shotgun.ammo = 500;
                g_Lara.magnums.ammo = 500;
                g_Lara.uzis.ammo = 5000;
                Sound_Effect(SFX_LARA_HOLSTER, NULL, SPM_ALWAYS);
            } else if (as == LS_SWAN_DIVE) {
                Effect_ExplodingDeath(g_Lara.item_number, -1, 0);
                Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
                g_LaraItem->hit_points = 0;
                g_LaraItem->flags |= IS_INVISIBLE;
            }
            cheat_mode = 0;
        }
        break;

    default:
        cheat_mode = 0;
        break;
    }
}

void Lara_EnterFlyMode(void)
{
    ITEM_INFO *const item = g_LaraItem;
    if (g_Lara.water_status != LWS_UNDERWATER || item->hit_points <= 0) {
        item->pos.y -= 0x80;
        item->current_anim_state = LS_SWIM;
        item->goal_anim_state = LS_SWIM;
        Item_SwitchToAnim(item, LA_SWIM_GLIDE, 0);
        item->gravity_status = 0;
        item->pos.x_rot = 30 * PHD_DEGREE;
        item->fall_speed = 30;
        g_Lara.head_x_rot = 0;
        g_Lara.head_y_rot = 0;
        g_Lara.torso_x_rot = 0;
        g_Lara.torso_y_rot = 0;
    }
    g_Lara.water_status = LWS_CHEAT;
    g_Lara.spaz_effect_count = 0;
    g_Lara.spaz_effect = NULL;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = -1;
    g_Lara.air = LARA_AIR;
    g_Lara.death_timer = 0;
    g_Lara.mesh_effects = 0;
    Lara_InitialiseMeshes(g_CurrentLevel);
}

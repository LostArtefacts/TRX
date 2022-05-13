#include "game/lara/lara_cheat.h"

#include "game/gameflow.h"
#include "game/inv.h"
#include "game/sound.h"
#include "global/vars.h"

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
            }
            cheat_mode = 0;
        }
        break;

    default:
        cheat_mode = 0;
        break;
    }
}

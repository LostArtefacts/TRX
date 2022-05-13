#include "game/control.h"

#include "game/camera.h"
#include "game/demo.h"
#include "game/gameflow.h"
#include "game/hair.h"
#include "game/input.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "global/vars.h"

#include <stddef.h>

static const int32_t m_AnimationRate = 0x8000;
static int32_t m_FrameCount = 0;

void CheckCheatMode(void)
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

int32_t ControlPhase(int32_t nframes, GAMEFLOW_LEVEL_TYPE level_type)
{
    int32_t return_val = 0;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }

    m_FrameCount += m_AnimationRate * nframes;
    while (m_FrameCount >= 0) {
        CheckCheatMode();
        if (g_LevelComplete) {
            return GF_NOP_BREAK;
        }

        Input_Update();

        if (level_type == GFL_DEMO) {
            if (g_Input.any) {
                return GF_EXIT_TO_TITLE;
            }
            if (!ProcessDemoInput()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        if (g_Lara.death_timer > DEATH_WAIT
            || (g_Lara.death_timer > DEATH_WAIT_MIN && g_Input.any
                && !g_Input.fly_cheat)
            || g_OverlayFlag == 2) {
            if (level_type == GFL_DEMO) {
                return GF_EXIT_TO_TITLE;
            }
            if (g_OverlayFlag == 2) {
                g_OverlayFlag = 1;
                return_val = Display_Inventory(INV_DEATH_MODE);
                if (return_val != GF_NOP) {
                    return return_val;
                }
            } else {
                g_OverlayFlag = 2;
            }
        }

        if ((g_InputDB.option || g_Input.save || g_Input.load
             || g_OverlayFlag <= 0)
            && !g_Lara.death_timer) {
            if (g_OverlayFlag > 0) {
                if (g_Input.load) {
                    g_OverlayFlag = -1;
                } else if (g_Input.save) {
                    g_OverlayFlag = -2;
                } else {
                    g_OverlayFlag = 0;
                }
            } else {
                if (g_OverlayFlag == -1) {
                    return_val = Display_Inventory(INV_LOAD_MODE);
                } else if (g_OverlayFlag == -2) {
                    return_val = Display_Inventory(INV_SAVE_MODE);
                } else {
                    return_val = Display_Inventory(INV_GAME_MODE);
                }

                g_OverlayFlag = 1;
                if (return_val != GF_NOP) {
                    return return_val;
                }
            }
        }

        if (!g_Lara.death_timer && g_InputDB.pause) {
            if (Control_Pause()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        int16_t item_num = g_NextItemActive;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &g_Items[item_num];
            OBJECT_INFO *obj = &g_Objects[item->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = item->next_active;
        }

        item_num = g_NextFxActive;
        while (item_num != NO_ITEM) {
            FX_INFO *fx = &g_Effects[item_num];
            OBJECT_INFO *obj = &g_Objects[fx->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = fx->next_active;
        }

        Lara_Control();
        Hair_Control(false);

        Camera_Update();
        Sound_UpdateEffects();
        g_GameInfo.current[g_CurrentLevel].stats.timer++;
        Overlay_BarHealthTimerTick();

        m_FrameCount -= 0x10000;
    }

    return GF_NOP;
}

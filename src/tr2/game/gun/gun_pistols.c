#include "game/gun/gun_pistols.h"

#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/gun/pistols.h>
#include <libtrx/game/math.h>

typedef enum {
    // clang-format off
    LA_PISTOLS_AIM    = 0,
    LA_PISTOLS_UNDRAW = 1,
    LA_PISTOLS_DRAW   = 2,
    LA_PISTOLS_RECOIL = 3,
    // clang-format on
} LARA_PISTOLS_ANIMATION;

static bool m_UziRight = false;
static bool m_UziLeft = false;

void Gun_Pistols_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
{
    int16_t anim_idx;
    if (Anim_TestAbsFrameRange(frame, LF_G_AIM_START, LF_G_AIM_END)) {
        anim_idx = LA_PISTOLS_AIM;
    } else if (Anim_TestAbsFrameRange(
                   frame, LF_G_UNDRAW_START, LF_G_UNDRAW_END)) {
        anim_idx = LA_PISTOLS_UNDRAW;
    } else if (Anim_TestAbsFrameRange(frame, LF_G_DRAW_START, LF_G_DRAW_END)) {
        anim_idx = LA_PISTOLS_DRAW;
    } else if (Anim_TestAbsFrameRange(
                   frame, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        anim_idx = LA_PISTOLS_RECOIL;
    } else {
        return;
    }

    const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    arm->anim_num = obj->anim_idx + anim_idx;
    arm->frame_num = frame;
    arm->frame_base = anim->frame_ptr;
}

void Gun_Pistols_Draw(const LARA_GUN_TYPE weapon_type)
{
    int16_t frame = g_Lara.left_arm.frame_num + 1;

    if (!(frame >= LF_G_UNDRAW_START && frame <= LF_G_DRAW_END)) {
        frame = LF_G_UNDRAW_START;
    } else if (frame == LF_G_DRAW_START) {
        Gun_Pistols_DrawMeshes(weapon_type);
        Sound_Effect(SFX_LARA_DRAW, &g_LaraItem->pos, SPM_NORMAL);
    } else if (frame == LF_G_DRAW_END) {
        Gun_Pistols_Ready(weapon_type);
        frame = LF_G_AIM_START;
    }

    Gun_Pistols_SetArmInfo(&g_Lara.right_arm, frame);
    Gun_Pistols_SetArmInfo(&g_Lara.left_arm, frame);
}

void Gun_Pistols_Undraw(const LARA_GUN_TYPE weapon_type)
{
    int16_t frame_l = g_Lara.left_arm.frame_num;
    if (frame_l >= LF_G_RECOIL_START && frame_l <= LF_G_RECOIL_END) {
        frame_l = LF_G_AIM_END;
    } else if (frame_l >= LF_G_AIM_BEND && frame_l <= LF_G_AIM_END) {
        g_Lara.left_arm.rot.x -= g_Lara.left_arm.rot.x / frame_l;
        g_Lara.left_arm.rot.y -= g_Lara.left_arm.rot.y / frame_l;
        frame_l--;
    } else if (frame_l == LF_G_AIM_START) {
        g_Lara.left_arm.rot.x = 0;
        g_Lara.left_arm.rot.y = 0;
        g_Lara.left_arm.rot.z = 0;
        frame_l = LF_G_DRAW_END;
    } else if (frame_l == LF_G_DRAW_START) {
        Gun_Pistols_UndrawMeshLeft(weapon_type);
        frame_l--;
    } else if (frame_l >= LF_G_UNDRAW_BEND && frame_l <= LF_G_DRAW_END) {
        frame_l--;
    }
    Gun_Pistols_SetArmInfo(&g_Lara.left_arm, frame_l);

    int16_t frame_r = g_Lara.right_arm.frame_num;
    if (frame_r >= LF_G_RECOIL_START && frame_r <= LF_G_RECOIL_END) {
        frame_r = LF_G_AIM_END;
    } else if (frame_r >= LF_G_AIM_BEND && frame_r <= LF_G_AIM_END) {
        g_Lara.right_arm.rot.x -= g_Lara.right_arm.rot.x / frame_r;
        g_Lara.right_arm.rot.y -= g_Lara.right_arm.rot.y / frame_r;
        frame_r--;
    } else if (frame_r == LF_G_AIM_START) {
        g_Lara.right_arm.rot.x = 0;
        g_Lara.right_arm.rot.y = 0;
        g_Lara.right_arm.rot.z = 0;
        frame_r = LF_G_DRAW_END;
    } else if (frame_r == LF_G_DRAW_START) {
        Gun_Pistols_UndrawMeshRight(weapon_type);
        frame_r--;
    } else if (frame_r >= LF_G_UNDRAW_BEND && frame_r <= LF_G_DRAW_END) {
        frame_r--;
    }
    Gun_Pistols_SetArmInfo(&g_Lara.right_arm, frame_r);

    if (frame_l == LF_G_UNDRAW_START && frame_r == LF_G_UNDRAW_START) {
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.target = nullptr;
        g_Lara.left_arm.frame_num = LF_G_AIM_START;
        g_Lara.left_arm.lock = 0;
        g_Lara.right_arm.frame_num = LF_G_AIM_START;
        g_Lara.right_arm.lock = 0;
    }

    if (!g_Input.look || g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x =
            (g_Lara.left_arm.rot.x + g_Lara.right_arm.rot.x) / 4;
        g_Lara.head_rot.y =
            (g_Lara.left_arm.rot.y + g_Lara.right_arm.rot.y) / 4;
        g_Lara.torso_rot.x = g_Lara.head_rot.x;
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    }
}

void Gun_Pistols_Ready(const LARA_GUN_TYPE weapon_type)
{
    g_Lara.gun_status = LGS_READY;
    g_Lara.target = nullptr;

    const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
    g_Lara.left_arm.frame_base = obj->frame_base;
    g_Lara.left_arm.frame_num = LF_G_AIM_START;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.right_arm.frame_base = obj->frame_base;
    g_Lara.right_arm.frame_num = LF_G_AIM_START;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;

    if (g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];

    if (g_Input.action) {
        Gun_TargetInfo(winfo);
    } else {
        g_Lara.target = nullptr;
    }

    if (g_Lara.target == nullptr) {
        Gun_GetNewTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);
    Gun_AimWeapon(winfo, &g_Lara.right_arm);

    if (g_Lara.left_arm.lock && !g_Lara.right_arm.lock) {
        g_Lara.head_rot.x = g_Lara.left_arm.rot.x / 2;
        g_Lara.head_rot.y = g_Lara.left_arm.rot.y / 2;
        g_Lara.torso_rot.x = g_Lara.head_rot.x;
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    } else if (!g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        g_Lara.head_rot.x = g_Lara.right_arm.rot.x / 2;
        g_Lara.head_rot.y = g_Lara.right_arm.rot.y / 2;
        g_Lara.torso_rot.x = g_Lara.head_rot.x;
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    } else if (g_Lara.right_arm.lock) {
        g_Lara.head_rot.x =
            (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
        g_Lara.head_rot.y =
            (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
        g_Lara.torso_rot.x = g_Lara.head_rot.x;
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    }

    Gun_Pistols_Animate(weapon_type);

    if (g_Lara.left_arm.flash_gun || g_Lara.right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

void Gun_Pistols_Animate(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];

    bool sound_already = false;
    int16_t angles[2];

    int32_t frame_r = g_Lara.right_arm.frame_num;
    if (!g_Lara.right_arm.lock
        && (!g_Input.action || g_Lara.target != nullptr)) {
        if (frame_r >= LF_G_RECOIL_START && frame_r <= LF_G_RECOIL_END) {
            frame_r = LF_G_AIM_END;
        } else if (frame_r >= LF_G_AIM_BEND && frame_r <= LF_G_AIM_END) {
            frame_r--;
        }
        if (m_UziRight) {
            Sound_Effect(winfo->sample_num + 1, &g_LaraItem->pos, SPM_NORMAL);
            m_UziRight = false;
        }
    } else if (frame_r >= LF_G_AIM_START && frame_r <= LF_G_AIM_EXTEND) {
        frame_r++;
    } else if (frame_r == LF_G_AIM_END) {
        if (g_Input.action) {
            angles[0] = g_Lara.right_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.right_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.right_arm.flash_gun = winfo->flash_time;
                Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
                sound_already = true;
                if (weapon_type == LGT_UZIS) {
                    m_UziRight = true;
                }
            }
            frame_r = LF_G_RECOIL_START;
        } else if (m_UziRight) {
            Sound_Effect(winfo->sample_num + 1, &g_LaraItem->pos, SPM_NORMAL);
            m_UziRight = false;
        }
    } else if (frame_r >= LF_G_RECOIL_START && frame_r <= LF_G_RECOIL_END) {
        frame_r++;
        if (frame_r == LF_G_RECOIL_START + winfo->recoil_frame) {
            frame_r = LF_G_AIM_END;
        }
        if (weapon_type == LGT_UZIS) {
            Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            m_UziRight = true;
        }
    }
    Gun_Pistols_SetArmInfo(&g_Lara.right_arm, frame_r);

    int16_t frame_l = g_Lara.left_arm.frame_num;
    if (!g_Lara.left_arm.lock
        && (!g_Input.action || g_Lara.target != nullptr)) {
        if (frame_l >= LF_G_RECOIL_START && frame_l <= LF_G_RECOIL_END) {
            frame_l = LF_G_AIM_END;
        } else if (frame_l >= LF_G_AIM_BEND && frame_l <= LF_G_AIM_END) {
            frame_l--;
        }
        if (m_UziLeft) {
            Sound_Effect(winfo->sample_num + 1, &g_LaraItem->pos, SPM_NORMAL);
            m_UziLeft = false;
        }
    } else if (frame_l >= LF_G_AIM_START && frame_l <= LF_G_AIM_EXTEND) {
        frame_l++;
    } else if (frame_l == LF_G_AIM_END) {
        if (g_Input.action) {
            angles[0] = g_Lara.left_arm.rot.y + g_LaraItem->rot.y;
            angles[1] = g_Lara.left_arm.rot.x;
            if (Gun_FireWeapon(
                    weapon_type, g_Lara.target, g_LaraItem, angles)) {
                g_Lara.left_arm.flash_gun = winfo->flash_time;
                if (!sound_already) {
                    Sound_Effect(
                        winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
                }
                if (weapon_type == LGT_UZIS) {
                    m_UziLeft = true;
                }
            }
            frame_l = LF_G_RECOIL_START;
        } else if (m_UziLeft) {
            Sound_Effect(winfo->sample_num + 1, &g_LaraItem->pos, SPM_NORMAL);
            m_UziLeft = false;
        }
    } else if (frame_l >= LF_G_RECOIL_START) {
        frame_l++;
        if (frame_l == LF_G_RECOIL_START + winfo->recoil_frame) {
            frame_l = LF_G_AIM_END;
        }
        if (weapon_type == LGT_UZIS) {
            Sound_Effect(winfo->sample_num, &g_LaraItem->pos, SPM_NORMAL);
            m_UziLeft = true;
        }
    }
    Gun_Pistols_SetArmInfo(&g_Lara.left_arm, frame_l);
}

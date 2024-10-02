#include "game/gun/gun_pistols.h"

#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "game/input.h"
#include "game/math.h"
#include "game/sound.h"
#include "global/funcs.h"
#include "global/vars.h"

static bool m_UziRight = false;
static bool m_UziLeft = false;

void __cdecl Gun_Pistols_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
{
    const int16_t anim_idx = g_Objects[O_LARA_PISTOLS].anim_idx;

    if (frame >= LF_G_AIM_START && frame <= LF_G_AIM_END) {
        arm->anim_num = anim_idx;
    } else if (frame >= LF_G_UNDRAW_START && frame <= LF_G_UNDRAW_END) {
        arm->anim_num = anim_idx + 1;
    } else if (frame >= LF_G_DRAW_START && frame <= LF_G_DRAW_END) {
        arm->anim_num = anim_idx + 2;
    } else if (frame >= LF_G_RECOIL_START && frame <= LF_G_RECOIL_END) {
        arm->anim_num = anim_idx + 3;
    }

    arm->frame_num = frame;
    arm->frame_base = g_Anims[arm->anim_num].frame_ptr;
}

void __cdecl Gun_Pistols_Draw(const LARA_GUN_TYPE weapon_type)
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

void __cdecl Gun_Pistols_Undraw(const LARA_GUN_TYPE weapon_type)
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
        Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
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
        Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
        frame_r--;
    } else if (frame_r >= LF_G_UNDRAW_BEND && frame_r <= LF_G_DRAW_END) {
        frame_r--;
    }
    Gun_Pistols_SetArmInfo(&g_Lara.right_arm, frame_r);

    if (frame_l == LF_G_UNDRAW_START && frame_r == LF_G_UNDRAW_START) {
        g_Lara.gun_status = LGS_ARMLESS;
        g_Lara.target = NULL;
        g_Lara.left_arm.frame_num = LF_G_AIM_START;
        g_Lara.left_arm.lock = 0;
        g_Lara.right_arm.frame_num = LF_G_AIM_START;
        g_Lara.right_arm.lock = 0;
    }

    if (!(g_Input & IN_LOOK)) {
        g_Lara.head_x_rot =
            (g_Lara.left_arm.rot.x + g_Lara.right_arm.rot.x) / 4;
        g_Lara.head_y_rot =
            (g_Lara.left_arm.rot.y + g_Lara.right_arm.rot.y) / 4;
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }
}

void __cdecl Gun_Pistols_Ready(const LARA_GUN_TYPE weapon_type)
{
    g_Lara.gun_status = LGS_READY;
    g_Lara.target = NULL;

    g_Lara.left_arm.frame_base = g_Objects[O_LARA_PISTOLS].frame_base;
    g_Lara.left_arm.frame_num = LF_G_AIM_START;
    g_Lara.left_arm.lock = 0;
    g_Lara.left_arm.rot.x = 0;
    g_Lara.left_arm.rot.y = 0;
    g_Lara.left_arm.rot.z = 0;
    g_Lara.right_arm.frame_base = g_Objects[O_LARA_PISTOLS].frame_base;
    g_Lara.right_arm.frame_num = LF_G_AIM_START;
    g_Lara.right_arm.lock = 0;
    g_Lara.right_arm.rot.x = 0;
    g_Lara.right_arm.rot.y = 0;
    g_Lara.right_arm.rot.z = 0;
}

void __cdecl Gun_Pistols_DrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(weapon_type);
    Gun_SetLaraHandRMesh(weapon_type);
    Gun_SetLaraHolsterLMesh(LGT_UNARMED);
    Gun_SetLaraHolsterRMesh(LGT_UNARMED);
}

void Gun_Pistols_UndrawMeshLeft(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHolsterLMesh(weapon_type);
}

void Gun_Pistols_UndrawMeshRight(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    Gun_SetLaraHolsterRMesh(weapon_type);
}

void __cdecl Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];

    if (g_Input & IN_ACTION) {
        Gun_TargetInfo(winfo);
    } else {
        g_Lara.target = NULL;
    }

    if (g_Lara.target == NULL) {
        Gun_GetNewTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);
    Gun_AimWeapon(winfo, &g_Lara.right_arm);

    if (g_Lara.left_arm.lock && !g_Lara.right_arm.lock) {
        g_Lara.head_x_rot = g_Lara.left_arm.rot.x / 2;
        g_Lara.head_y_rot = g_Lara.left_arm.rot.y / 2;
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    } else if (!g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        g_Lara.head_x_rot = g_Lara.right_arm.rot.x / 2;
        g_Lara.head_y_rot = g_Lara.right_arm.rot.y / 2;
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    } else if (g_Lara.right_arm.lock) {
        g_Lara.head_x_rot =
            (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
        g_Lara.head_y_rot =
            (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }

    Gun_Pistols_Animate(weapon_type);

    if (g_Lara.left_arm.flash_gun || g_Lara.right_arm.flash_gun) {
        const int32_t c = Math_Cos(g_LaraItem->rot.y);
        const int32_t s = Math_Sin(g_LaraItem->rot.y);
        const int32_t x = g_LaraItem->pos.x + (s >> (W2V_SHIFT - 10));
        const int32_t y = g_LaraItem->pos.y - WALL_L / 2;
        const int32_t z = g_LaraItem->pos.z + (c >> (W2V_SHIFT - 10));
        AddDynamicLight(x, y, z, 12, 11);
    }
}

void __cdecl Gun_Pistols_Animate(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const winfo = &g_Weapons[weapon_type];

    bool sound_already = false;
    int16_t angles[2];

    int32_t frame_r = g_Lara.right_arm.frame_num;
    if (!g_Lara.right_arm.lock
        && (!(g_Input & IN_ACTION) || g_Lara.target != NULL)) {
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
        if (g_Input & IN_ACTION) {
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
        && (!(g_Input & IN_ACTION) || g_Lara.target != NULL)) {
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
        if (g_Input & IN_ACTION) {
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

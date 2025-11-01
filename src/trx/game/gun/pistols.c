#include <trx/game/gun/pistols.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/control.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/vars.h>
#include <trx/game/lara/common.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_ENABLE_FAST_UZI (g_TRVersion == 2)

typedef enum {
    // clang-format off
    LA_PISTOLS_AIM    = 0,
    LA_PISTOLS_UNDRAW = 1,
    LA_PISTOLS_DRAW   = 2,
    LA_PISTOLS_RECOIL = 3,
    // clang-format on
} LARA_PISTOLS_ANIMATION;

static bool m_SoundRight = false;
static bool m_SoundLeft = false;

static bool M_EnableFastSound(const LARA_GUN_TYPE weapon_type)
{
    return g_TRVersion == 2 && weapon_type == LGT_UZIS;
}

static void M_FireSound(const SAMPLE_TRX_ID sample_trx_id, const bool alternate)
{
    SAMPLE_ID sample_id = Sound_ToGameID(sample_trx_id);
    if (sample_id == SFX_INVALID) {
        return;
    }
    if (alternate) {
        sample_id += 1;
    }
    Sound_Effect_Direct(sample_id, &Lara_GetItem()->pos, SPM_NORMAL);
}

static void M_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
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

static void M_Animate(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    const ITEM *const lara_item = Lara_GetItem();

    bool sound_already = false;
    int16_t angles[2];

    int32_t frame_r = lara->right_arm.frame_num;
    if (!lara->right_arm.lock && (!g_Input.action || lara->target != nullptr)) {
        if (Anim_TestAbsFrameRange(
                frame_r, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            frame_r = LF_G_AIM_END;
        } else if (Anim_TestAbsFrameRange(
                       frame_r, LF_G_AIM_BEND, LF_G_AIM_END)) {
            frame_r--;
        }
        if (m_SoundRight) {
            M_FireSound(weapon->sample_num, true);
            m_SoundRight = false;
        }
    } else {
        if (Anim_TestAbsFrameRange(frame_r, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
            frame_r++;
        } else if (frame_r == LF_G_AIM_END) {
            if (g_Input.action) {
                angles[0] = lara->right_arm.rot.y + lara_item->rot.y;
                angles[1] = lara->right_arm.rot.x;
                if (Gun_FireWeapon(
                        weapon_type, lara->target, lara_item, angles)) {
                    lara->right_arm.flash_gun = weapon->flash_time;
                    if (!sound_already) {
                        M_FireSound(weapon->sample_num, false);
                    }
                    sound_already = true;
                    if (M_EnableFastSound(weapon_type)) {
                        m_SoundRight = true;
                    }
                }
                frame_r = LF_G_RECOIL_START;
            } else if (m_SoundRight) {
                M_FireSound(weapon->sample_num, true);
                m_SoundRight = false;
            }
        } else if (Anim_TestAbsFrameRange(
                       frame_r, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            frame_r++;
            if (frame_r == LF_G_RECOIL_START + weapon->recoil_frame) {
                frame_r = LF_G_AIM_END;
            }
            if (M_EnableFastSound(weapon_type)) {
                M_FireSound(weapon->sample_num, false);
                m_SoundRight = true;
            }
        }
    }
    M_SetArmInfo(&lara->right_arm, frame_r);

    int16_t frame_l = lara->left_arm.frame_num;
    if (!lara->left_arm.lock && (!g_Input.action || lara->target != nullptr)) {
        if (Anim_TestAbsFrameRange(
                frame_l, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
            frame_l = LF_G_AIM_END;
        } else if (Anim_TestAbsFrameRange(
                       frame_l, LF_G_AIM_BEND, LF_G_AIM_END)) {
            frame_l--;
        }
        if (m_SoundLeft) {
            M_FireSound(weapon->sample_num, true);
            m_SoundLeft = false;
        }
    } else if (Anim_TestAbsFrameRange(
                   frame_l, LF_G_AIM_START, LF_G_AIM_EXTEND)) {
        frame_l++;
    } else if (frame_l == LF_G_AIM_END) {
        if (g_Input.action) {
            angles[0] = lara->left_arm.rot.y + lara_item->rot.y;
            angles[1] = lara->left_arm.rot.x;
            if (Gun_FireWeapon(weapon_type, lara->target, lara_item, angles)) {
                lara->left_arm.flash_gun = weapon->flash_time;
                if (!sound_already) {
                    M_FireSound(weapon->sample_num, false);
                }
                if (M_EnableFastSound(weapon_type)) {
                    m_SoundLeft = true;
                }
            }
            frame_l = LF_G_RECOIL_START;
        } else if (m_SoundLeft) {
            M_FireSound(weapon->sample_num, true);
            m_SoundLeft = false;
        }
    } else if (Anim_TestAbsFrameRange(
                   frame_l, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        frame_l++;
        if (frame_l == LF_G_RECOIL_START + weapon->recoil_frame) {
            frame_l = LF_G_AIM_END;
        }
        if (M_EnableFastSound(weapon_type)) {
            M_FireSound(weapon->sample_num, false);
            m_SoundLeft = true;
        }
    }
    M_SetArmInfo(&lara->left_arm, frame_l);
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    const WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Gun_GetNewTarget(weapon);
    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(weapon);
    }

    Gun_AimWeapon(weapon, &lara->left_arm);
    Gun_AimWeapon(weapon, &lara->right_arm);

    const bool lock_head = g_Config.gameplay.look_mode != LOOK_MODE_UNRESTRICTED
        || g_Camera.type != CAM_LOOK;
    if (lara->left_arm.lock && !lara->right_arm.lock) {
        if (lock_head) {
            lara->head_rot.x = lara->left_arm.rot.x / 2;
            lara->head_rot.y = lara->left_arm.rot.y / 2;
        }
        lara->torso_rot.x = lara->left_arm.rot.x / 2;
        lara->torso_rot.y = lara->left_arm.rot.y / 2;
    } else if (!lara->left_arm.lock && lara->right_arm.lock) {
        if (lock_head) {
            lara->head_rot.x = lara->right_arm.rot.x / 2;
            lara->head_rot.y = lara->right_arm.rot.y / 2;
        }
        lara->torso_rot.x = lara->right_arm.rot.x / 2;
        lara->torso_rot.y = lara->right_arm.rot.y / 2;
    } else if (lara->right_arm.lock) {
        if (lock_head) {
            lara->head_rot.x =
                (lara->right_arm.rot.x + lara->left_arm.rot.x) / 4;
            lara->head_rot.y =
                (lara->right_arm.rot.y + lara->left_arm.rot.y) / 4;
        }
        lara->torso_rot.x = (lara->right_arm.rot.x + lara->left_arm.rot.x) / 4;
        lara->torso_rot.y = (lara->right_arm.rot.y + lara->left_arm.rot.y) / 4;
    }

    M_Animate(weapon_type);

    if (lara->left_arm.flash_gun || lara->right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

void Gun_Pistols_Draw(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    int16_t frame = lara->left_arm.frame_num + 1;

    if (!Anim_TestAbsFrameRange(frame, LF_G_UNDRAW_START, LF_G_DRAW_END)) {
        frame = LF_G_UNDRAW_START;
    } else if (Anim_TestAbsFrameEqual(frame, LF_G_DRAW_START)) {
        Gun_Pistols_DrawMeshes(weapon_type);
        Sound_Effect(SFX_LARA_DRAW, &Lara_GetItem()->pos, SPM_NORMAL);
    } else if (Anim_TestAbsFrameEqual(frame, LF_G_DRAW_END)) {
        Gun_Pistols_Ready(weapon_type);
        frame = LF_G_AIM_START;
    }

    M_SetArmInfo(&lara->right_arm, frame);
    M_SetArmInfo(&lara->left_arm, frame);
}

void Gun_Pistols_Undraw(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();

    int16_t frame_l = lara->left_arm.frame_num;
    if (Anim_TestAbsFrameRange(frame_l, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        frame_l = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(frame_l, LF_G_AIM_BEND, LF_G_AIM_END)) {
        lara->left_arm.rot.x -= lara->left_arm.rot.x / frame_l;
        lara->left_arm.rot.y -= lara->left_arm.rot.y / frame_l;
        frame_l--;
    } else if (Anim_TestAbsFrameEqual(frame_l, LF_G_AIM_START)) {
        lara->left_arm.rot.x = 0;
        lara->left_arm.rot.y = 0;
        lara->left_arm.rot.z = 0;
        frame_l = LF_G_DRAW_END;
    } else if (Anim_TestAbsFrameEqual(frame_l, LF_G_DRAW_START)) {
        Gun_Pistols_UndrawMeshLeft(weapon_type);
        frame_l--;
    } else if (Anim_TestAbsFrameRange(
                   frame_l, LF_G_UNDRAW_BEND, LF_G_DRAW_END)) {
        frame_l--;
    }
    M_SetArmInfo(&lara->left_arm, frame_l);

    int16_t frame_r = lara->right_arm.frame_num;
    if (Anim_TestAbsFrameRange(frame_r, LF_G_RECOIL_START, LF_G_RECOIL_END)) {
        frame_r = LF_G_AIM_END;
    } else if (Anim_TestAbsFrameRange(frame_r, LF_G_AIM_BEND, LF_G_AIM_END)) {
        lara->right_arm.rot.x -= lara->right_arm.rot.x / frame_r;
        lara->right_arm.rot.y -= lara->right_arm.rot.y / frame_r;
        frame_r--;
    } else if (Anim_TestAbsFrameEqual(frame_r, LF_G_AIM_START)) {
        lara->right_arm.rot.x = 0;
        lara->right_arm.rot.y = 0;
        lara->right_arm.rot.z = 0;
        frame_r = LF_G_DRAW_END;
    } else if (Anim_TestAbsFrameEqual(frame_r, LF_G_DRAW_START)) {
        Gun_Pistols_UndrawMeshRight(weapon_type);
        frame_r--;
    } else if (Anim_TestAbsFrameRange(
                   frame_r, LF_G_UNDRAW_BEND, LF_G_DRAW_END)) {
        frame_r--;
    }
    M_SetArmInfo(&lara->right_arm, frame_r);

    if (Anim_TestAbsFrameEqual(frame_l, LF_G_UNDRAW_START)
        && Anim_TestAbsFrameEqual(frame_r, LF_G_UNDRAW_START)) {
        lara->gun_status = LGS_ARMLESS;
        lara->left_arm.lock = 0;
        lara->right_arm.lock = 0;
        lara->left_arm.frame_num = LF_G_AIM_START;
        lara->right_arm.frame_num = LF_G_AIM_START;
        lara->target = nullptr;
    }

    if (!g_Input.look || g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        lara->head_rot.x = (lara->left_arm.rot.x + lara->right_arm.rot.x) / 4;
        lara->head_rot.y = (lara->left_arm.rot.y + lara->right_arm.rot.y) / 4;
        lara->torso_rot.x = lara->head_rot.x;
        lara->torso_rot.y = lara->head_rot.y;
    }
}

void Gun_Pistols_Ready(const LARA_GUN_TYPE weapon_type)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_READY;
    lara->target = nullptr;

    const OBJECT *const obj = Object_Get(O_LARA_PISTOLS);
    lara->left_arm.frame_base = obj->frame_base;
    lara->left_arm.frame_num = LF_G_AIM_START;
    lara->left_arm.lock = 0;
    lara->left_arm.rot.x = 0;
    lara->left_arm.rot.y = 0;
    lara->left_arm.rot.z = 0;
    lara->right_arm.frame_base = obj->frame_base;
    lara->right_arm.frame_num = LF_G_AIM_START;
    lara->right_arm.lock = 0;
    lara->right_arm.rot.x = 0;
    lara->right_arm.rot.y = 0;
    lara->right_arm.rot.z = 0;

    if (g_Config.gameplay.look_mode == LOOK_MODE_RESTRICTED) {
        lara->head_rot.x = 0;
        lara->head_rot.y = 0;
        lara->torso_rot.x = 0;
        lara->torso_rot.y = 0;
    }
}

void Gun_Pistols_DrawMeshes(const LARA_GUN_TYPE weapon_type)
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
    Sound_Effect(SFX_LARA_HOLSTER, &Lara_GetItem()->pos, SPM_NORMAL);
}

void Gun_Pistols_UndrawMeshRight(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    Gun_SetLaraHolsterRMesh(weapon_type);
    Sound_Effect(SFX_LARA_HOLSTER, &Lara_GetItem()->pos, SPM_NORMAL);
}

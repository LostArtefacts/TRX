#include "game/gun/pistols.h"

#include "config.h"
#include "game/gun/common.h"
#include "game/lara/common.h"
#include "game/sound.h"

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

    Gun_Pistols_SetArmInfo(&lara->right_arm, frame);
    Gun_Pistols_SetArmInfo(&lara->left_arm, frame);
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
    Gun_Pistols_SetArmInfo(&lara->left_arm, frame_l);

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
    Gun_Pistols_SetArmInfo(&lara->right_arm, frame_r);

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

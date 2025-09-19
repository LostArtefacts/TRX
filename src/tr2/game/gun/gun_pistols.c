#include "game/gun/gun_pistols.h"

#include "game/gun/gun_misc.h"

#include <libtrx/config.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/lara.h>

typedef enum {
    // clang-format off
    LA_PISTOLS_AIM    = 0,
    LA_PISTOLS_UNDRAW = 1,
    LA_PISTOLS_DRAW   = 2,
    LA_PISTOLS_RECOIL = 3,
    // clang-format on
} LARA_PISTOLS_ANIMATION;

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

    if (lara->left_arm.lock && !lara->right_arm.lock) {
        lara->head_rot.x = lara->left_arm.rot.x / 2;
        lara->head_rot.y = lara->left_arm.rot.y / 2;
        lara->torso_rot.x = lara->head_rot.x;
        lara->torso_rot.y = lara->head_rot.y;
    } else if (!lara->left_arm.lock && lara->right_arm.lock) {
        lara->head_rot.x = lara->right_arm.rot.x / 2;
        lara->head_rot.y = lara->right_arm.rot.y / 2;
        lara->torso_rot.x = lara->head_rot.x;
        lara->torso_rot.y = lara->head_rot.y;
    } else if (lara->right_arm.lock) {
        lara->head_rot.x = (lara->right_arm.rot.x + lara->left_arm.rot.x) / 4;
        lara->head_rot.y = (lara->right_arm.rot.y + lara->left_arm.rot.y) / 4;
        lara->torso_rot.x = lara->head_rot.x;
        lara->torso_rot.y = lara->head_rot.y;
    }

    Gun_Pistols_Animate(weapon_type);

    if (lara->left_arm.flash_gun || lara->right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

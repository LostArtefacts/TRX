#include "game/gun/gun_pistols.h"

#include "game/gun/gun.h"
#include "game/gun/gun_misc.h"
#include "global/vars.h"

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

#include "game/gun/gun_pistols.h"

#include "game/gun.h"
#include "game/gun/gun_misc.h"
#include "global/vars.h"

#include <libtrx/config.h>

void Gun_Pistols_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
{
    arm->frame_num = frame;
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *winfo = &g_Weapons[weapon_type];

    Gun_GetNewTarget(winfo);

    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(winfo);
    }

    Gun_AimWeapon(winfo, &g_Lara.left_arm);
    Gun_AimWeapon(winfo, &g_Lara.right_arm);

    if (g_Lara.left_arm.lock && !g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x = g_Lara.left_arm.rot.x / 2;
            g_Lara.head_rot.y = g_Lara.left_arm.rot.y / 2;
        }
        g_Lara.torso_rot.x = g_Lara.left_arm.rot.x / 2;
        g_Lara.torso_rot.y = g_Lara.left_arm.rot.y / 2;
    } else if (!g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x = g_Lara.right_arm.rot.x / 2;
            g_Lara.head_rot.y = g_Lara.right_arm.rot.y / 2;
        }
        g_Lara.torso_rot.x = g_Lara.right_arm.rot.x / 2;
        g_Lara.torso_rot.y = g_Lara.right_arm.rot.y / 2;
    } else if (g_Lara.left_arm.lock && g_Lara.right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            g_Lara.head_rot.x =
                (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
            g_Lara.head_rot.y =
                (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
        }
        g_Lara.torso_rot.x =
            (g_Lara.right_arm.rot.x + g_Lara.left_arm.rot.x) / 4;
        g_Lara.torso_rot.y =
            (g_Lara.right_arm.rot.y + g_Lara.left_arm.rot.y) / 4;
    }

    Gun_Pistols_Animate(weapon_type);

    if (g_Lara.left_arm.flash_gun || g_Lara.right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

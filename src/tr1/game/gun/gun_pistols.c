#include "game/gun/gun_pistols.h"

#include "game/gun/gun_misc.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/lara.h>

void Gun_Pistols_SetArmInfo(LARA_ARM *const arm, const int32_t frame)
{
    arm->frame_num = frame;
}

void Gun_Pistols_Control(const LARA_GUN_TYPE weapon_type)
{
    WEAPON_INFO *const weapon = &g_Weapons[weapon_type];
    LARA_INFO *const lara = Lara_GetLaraInfo();

    Gun_GetNewTarget(weapon);
    if (g_InputDB.change_target && g_Config.gameplay.enable_target_change) {
        Gun_ChangeTarget(weapon);
    }

    Gun_AimWeapon(weapon, &lara->left_arm);
    Gun_AimWeapon(weapon, &lara->right_arm);

    if (lara->left_arm.lock && !lara->right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            lara->head_rot.x = lara->left_arm.rot.x / 2;
            lara->head_rot.y = lara->left_arm.rot.y / 2;
        }
        lara->torso_rot.x = lara->left_arm.rot.x / 2;
        lara->torso_rot.y = lara->left_arm.rot.y / 2;
    } else if (!lara->left_arm.lock && lara->right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            lara->head_rot.x = lara->right_arm.rot.x / 2;
            lara->head_rot.y = lara->right_arm.rot.y / 2;
        }
        lara->torso_rot.x = lara->right_arm.rot.x / 2;
        lara->torso_rot.y = lara->right_arm.rot.y / 2;
    } else if (lara->left_arm.lock && lara->right_arm.lock) {
        if (g_Camera.type != CAM_LOOK) {
            lara->head_rot.x =
                (lara->right_arm.rot.x + lara->left_arm.rot.x) / 4;
            lara->head_rot.y =
                (lara->right_arm.rot.y + lara->left_arm.rot.y) / 4;
        }
        lara->torso_rot.x = (lara->right_arm.rot.x + lara->left_arm.rot.x) / 4;
        lara->torso_rot.y = (lara->right_arm.rot.y + lara->left_arm.rot.y) / 4;
    }

    Gun_Pistols_Animate(weapon_type);

    if (lara->left_arm.flash_gun || lara->right_arm.flash_gun) {
        Gun_AddDynamicLight();
    }
}

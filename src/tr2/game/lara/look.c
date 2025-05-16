#include "game/lara/look.h"

#include "game/input.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>

void Lara_LookUpDown(void)
{
    g_Camera.type = CAM_LOOK;

    if (g_Input.forward) {
        g_Input.forward = 0;
        if (g_Lara.head_rot.x > MIN_HEAD_TILT) {
            g_Lara.head_rot.x -= HEAD_TURN;
        }
    } else if (g_Input.back) {
        g_Input.back = 0;
        if (g_Lara.head_rot.x < MAX_HEAD_TILT) {
            g_Lara.head_rot.x += HEAD_TURN;
        }
    }

    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_rot.x = g_Lara.head_rot.x;
    }
}

void Lara_LookLeftRight(void)
{
    g_Camera.type = CAM_LOOK;

    if (g_Input.left) {
        g_Input.left = 0;
        if (g_Lara.head_rot.y > MIN_HEAD_ROTATION)
            g_Lara.head_rot.y -= HEAD_TURN;
    } else if (g_Input.right) {
        g_Input.right = 0;
        if (g_Lara.head_rot.y < MAX_HEAD_ROTATION)
            g_Lara.head_rot.y += HEAD_TURN;
    }

    if (g_Lara.gun_status != LGS_HANDS_BUSY && g_Lara.skidoo == NO_ITEM) {
        g_Lara.torso_rot.y = g_Lara.head_rot.y;
    }
}

void Lara_ResetLook(void)
{
    if (g_Camera.type == CAM_LOOK) {
        return;
    }

    if (g_Lara.head_rot.x <= -HEAD_TURN || g_Lara.head_rot.x >= HEAD_TURN) {
        g_Lara.head_rot.x -= g_Lara.head_rot.x / 8;
    } else {
        g_Lara.head_rot.x = 0;
    }

    if (g_Lara.head_rot.y <= -HEAD_TURN || g_Lara.head_rot.y >= HEAD_TURN) {
        g_Lara.head_rot.y += g_Lara.head_rot.y / -8;
    } else {
        g_Lara.head_rot.y = 0;
    }

    g_Lara.torso_rot.x = g_Lara.head_rot.x;
    g_Lara.torso_rot.y = g_Lara.head_rot.y;
}

#include "game/lara/look.h"

#include "game/input.h"
#include "global/const.h"
#include "global/vars.h"

void __cdecl Lara_LookUpDown(void)
{
    g_Camera.type = CAM_LOOK;

    if (g_Input & IN_FORWARD) {
        g_Input &= ~IN_FORWARD;
        if (g_Lara.head_x_rot > MIN_HEAD_TILT) {
            g_Lara.head_x_rot -= HEAD_TURN;
        }
    } else if (g_Input & IN_BACK) {
        g_Input &= ~IN_BACK;
        if (g_Lara.head_x_rot < MAX_HEAD_TILT) {
            g_Lara.head_x_rot += HEAD_TURN;
        }
    }

    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
    }
}

void __cdecl Lara_LookLeftRight(void)
{
    g_Camera.type = CAM_LOOK;

    if (g_Input & IN_LEFT) {
        g_Input &= ~IN_LEFT;
        if (g_Lara.head_y_rot > MIN_HEAD_ROTATION)
            g_Lara.head_y_rot -= HEAD_TURN;
    } else if (g_Input & IN_RIGHT) {
        g_Input &= ~IN_RIGHT;
        if (g_Lara.head_y_rot < MAX_HEAD_ROTATION)
            g_Lara.head_y_rot += HEAD_TURN;
    }

    if (g_Lara.gun_status != LGS_HANDS_BUSY && g_Lara.skidoo == NO_ITEM) {
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }
}

void __cdecl Lara_ResetLook(void)
{
    if (g_Camera.type == CAM_LOOK) {
        return;
    }

    if (g_Lara.head_x_rot <= -HEAD_TURN || g_Lara.head_x_rot >= HEAD_TURN) {
        g_Lara.head_x_rot -= g_Lara.head_x_rot / 8;
    } else {
        g_Lara.head_x_rot = 0;
    }

    if (g_Lara.head_y_rot <= -HEAD_TURN || g_Lara.head_y_rot >= HEAD_TURN) {
        g_Lara.head_y_rot += g_Lara.head_y_rot / -8;
    } else {
        g_Lara.head_y_rot = 0;
    }

    g_Lara.torso_x_rot = g_Lara.head_x_rot;
    g_Lara.torso_y_rot = g_Lara.head_y_rot;
}

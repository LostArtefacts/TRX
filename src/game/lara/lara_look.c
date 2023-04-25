#include "game/lara/lara_look.h"

#include "game/input.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdint.h>

void Lara_LookLeftRight(int16_t max_head_rot, int16_t head_turn)
{
    g_Camera.type = CAM_LOOK;
    if (g_Input.left) {
        g_Input.left = 0;
        if (g_Lara.head_y_rot > -max_head_rot) {
            g_Lara.head_y_rot -= head_turn / 2;
        }
    } else if (g_Input.right) {
        g_Input.right = 0;
        if (g_Lara.head_y_rot < max_head_rot) {
            g_Lara.head_y_rot += head_turn / 2;
        }
    }
    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }
}

void Lara_LookLeftRightSurf(int16_t max_head_rot, int16_t head_turn)
{
    g_Camera.type = CAM_LOOK;
    if (g_Input.left) {
        g_Input.left = 0;
        if (g_Lara.head_y_rot > -max_head_rot) {
            g_Lara.head_y_rot -= head_turn;
        }
    } else if (g_Input.right) {
        g_Input.right = 0;
        if (g_Lara.head_y_rot < max_head_rot) {
            g_Lara.head_y_rot += head_turn;
        }
    }
    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_y_rot = g_Lara.head_y_rot;
    }
    g_Lara.torso_y_rot = g_Lara.head_y_rot / 2;
}

void Lara_LookUpDown(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn)
{
    g_Camera.type = CAM_LOOK;
    if (g_Input.forward) {
        g_Input.forward = 0;
        if (g_Lara.head_x_rot > min_head_tilt) {
            g_Lara.head_x_rot -= head_turn / 2;
        }
    } else if (g_Input.back) {
        g_Input.back = 0;
        if (g_Lara.head_x_rot < max_head_tilt) {
            g_Lara.head_x_rot += head_turn / 2;
        }
    }
    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
    }
}

void Lara_LookUpDownSurf(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn)
{
    g_Camera.type = CAM_LOOK;
    if (g_Input.forward) {
        g_Input.forward = 0;
        if (g_Lara.head_x_rot > min_head_tilt) {
            g_Lara.head_x_rot -= head_turn;
        }
    } else if (g_Input.back) {
        g_Input.back = 0;
        if (g_Lara.head_x_rot < max_head_tilt) {
            g_Lara.head_x_rot += head_turn;
        }
    }
    if (g_Lara.gun_status != LGS_HANDS_BUSY) {
        g_Lara.torso_x_rot = g_Lara.head_x_rot;
    }
    g_Lara.torso_x_rot = 0;
}

void Lara_ResetLook(void)
{
    if (g_Camera.type == CAM_LOOK) {
        return;
    }
    if (g_Lara.head_x_rot <= -HEAD_TURN / 2
        || g_Lara.head_x_rot >= HEAD_TURN / 2) {
        g_Lara.head_x_rot = g_Lara.head_x_rot / -8 + g_Lara.head_x_rot;
    } else {
        g_Lara.head_x_rot = 0;
    }
    g_Lara.torso_x_rot = g_Lara.head_x_rot;

    if (g_Lara.head_y_rot <= -HEAD_TURN / 2
        || g_Lara.head_y_rot >= HEAD_TURN / 2) {
        g_Lara.head_y_rot += g_Lara.head_y_rot / -8;
    } else {
        g_Lara.head_y_rot = 0;
    }
    g_Lara.torso_y_rot = g_Lara.head_y_rot;
}

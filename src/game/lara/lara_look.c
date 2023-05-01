#include "game/lara/lara_look.h"

#include "game/input.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdint.h>

static void Lara_LookLeftRightBase(int16_t max_head_rot, int16_t head_turn);
static void Lara_LookUpDownBase(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn);

static void Lara_LookLeftRightBase(int16_t max_head_rot, int16_t head_turn)
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
}

static void Lara_LookUpDownBase(
    int16_t min_head_tilt, int16_t max_head_tilt, int16_t head_turn)
{
    if (g_Config.enabled_inverted_look) {
        uint16_t temp_forward = g_Input.forward;
        g_Input.forward = g_Input.back;
        g_Input.back = temp_forward;
    }

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
}

void Lara_LookLeftRight(void)
{
    Lara_LookLeftRightBase(MAX_HEAD_ROTATION, HEAD_TURN / 2);
}

void Lara_LookLeftRightSurf(void)
{
    Lara_LookLeftRightBase(MAX_HEAD_ROTATION_SURF, HEAD_TURN_SURF);
    g_Lara.torso_y_rot = g_Lara.head_y_rot / 2;
}

void Lara_LookUpDown(void)
{
    Lara_LookUpDownBase(MIN_HEAD_TILT_LOOK, MAX_HEAD_TILT_LOOK, HEAD_TURN / 2);
}

void Lara_LookUpDownSurf(void)
{
    Lara_LookUpDownBase(MIN_HEAD_TILT_SURF, MAX_HEAD_TILT_SURF, HEAD_TURN_SURF);
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

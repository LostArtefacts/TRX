#include "global/vars.h"

#include "game/spawn.h"

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR2X (non-Docker build)";
#endif

const float g_RhwFactor = 0x14000000.p0;

uint32_t g_PerspectiveDistance = 0x3000000;

int32_t g_OverlayStatus = 1;
int32_t g_MidSort = 0;

float g_FltResZBuf;
float g_FltResZ;
int32_t g_PhdPersp;

int32_t g_PhdTop;
int32_t g_PhdLeft;
int32_t g_PhdRight;
int32_t g_PhdBottom;

LARA_INFO g_Lara;
ITEM *g_LaraItem = nullptr;

WEAPON_INFO g_Weapons[] = {
    {},
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 8,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 2,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 21,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8192,
        .recoil_frame = 3,
        .flash_time = 3,
        .sample_num = 43,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 0,
        .gun_height = 500,
        .damage = 3,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 45,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 728,
        .gun_height = 500,
        .damage = 3,
        .target_dist = 12288,
        .recoil_frame = 0,
        .flash_time = 3,
        .sample_num = 0,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 500,
        .damage = 30,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -75 * DEG_1, 75 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -75 * DEG_1, 75 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 500,
        .damage = 4,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },
    {},
    {
        .lock_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .right_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 400,
        .damage = 3,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 43,
    },
};

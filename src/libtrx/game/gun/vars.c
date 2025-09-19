#include "game/gun/vars.h"

#include "game/const.h"
#include "game/sound.h"

WEAPON_INFO g_Weapons[NUM_WEAPONS] = {
    [LGT_UNARMED] = {
        .sample_num = SFX_LARA_NO,
    },

    [LGT_PISTOLS] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -60 * DEG_1, +60 * DEG_1 },
        .left_angles = { -170 * DEG_1, +60 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .right_angles = { -60 * DEG_1, +170 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = SFX_LARA_PISTOLS,
    },

    [LGT_MAGNUMS] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -60 * DEG_1, +60 * DEG_1 },
        .left_angles = { -170 * DEG_1, +60 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .right_angles = { -60 * DEG_1, +170 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 650,
        .damage = 2,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = SFX_LARA_MAGNUMS,
    },

    [LGT_UZIS] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -60 * DEG_1, +60 * DEG_1 },
        .left_angles = { -170 * DEG_1, +60 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .right_angles = { -60 * DEG_1, +170 * DEG_1, -80 * DEG_1, +80 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 3,
#if TR_VERSION == 1
        .flash_time = 2,
#else
        .flash_time = 3,
#endif
        .sample_num = SFX_LARA_UZI_FIRE,
    },

    [LGT_SHOTGUN] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -55 * DEG_1, +55 * DEG_1 },
        .left_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .right_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 0,
        .gun_height = 500,
#if TR_VERSION == 1
        .damage = 4,
#else
        .damage = 3,
#endif
        .target_dist = 8 * WALL_L,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = SFX_LARA_SHOTGUN,
    },

#if TR_VERSION >= 2
    [LGT_M16] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -55 * DEG_1, +55 * DEG_1 },
        .left_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .right_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 4 * DEG_1,
        .gun_height = 500,
        .damage = 3,
        .target_dist = 12 * WALL_L,
        .recoil_frame = 0,
        .flash_time = 3,
        .sample_num = 0,
    },

    [LGT_GRENADE] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -55 * DEG_1, +55 * DEG_1 },
        .left_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .right_angles = { -80 * DEG_1, +80 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 500,
        .damage = 30,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },

    [LGT_HARPOON] = {
        .lock_angles = { -60 * DEG_1, +60 * DEG_1, -65 * DEG_1, +65 * DEG_1 },
        .left_angles = { -80 * DEG_1, +80 * DEG_1, -75 * DEG_1, +75 * DEG_1 },
        .right_angles = { -80 * DEG_1, +80 * DEG_1, -75 * DEG_1, +75 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 500,
        .damage = 4,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },

    [LGT_FLARE] = {},

    [LGT_SKIDOO] = {
        .lock_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .right_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .aim_speed = 10 * DEG_1,
        .shot_accuracy = 8 * DEG_1,
        .gun_height = 400,
        .damage = 3,
        .target_dist = 8 * WALL_L,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = SFX_LARA_UZI_FIRE,
    },
#endif
};

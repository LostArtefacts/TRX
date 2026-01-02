#pragma once

#include <trx/game/sound/ids.h>

#include <stdint.h>

typedef enum {
    WEAPON_TYPE_DUAL_PISTOLS,
    WEAPON_TYPE_SINGLE_PISTOL,
    WEAPON_TYPE_RIFLE,
    WEAPON_TYPE_MOUNTED,
    NUM_WEAPON_TYPES,
} WEAPON_TYPE;

typedef struct {
    WEAPON_TYPE type;
    int16_t lock_angles[4];
    int16_t left_angles[4];
    int16_t right_angles[4];
    int16_t aim_speed;
    int16_t shot_accuracy;
    int32_t gun_height;
    int32_t damage;
    int32_t target_dist;
    int16_t equip_anim_idx;
    int16_t draw_frame;
    int16_t undraw_frame;
    int16_t recoil_frame;
    int16_t flash_time;
    SAMPLE_TRX_ID sample_num;
    bool is_available;
} WEAPON_INFO;

#pragma once

#include "../anims.h"
#include "../creature.h"
#include "../effects/types.h"
#include "../items/types.h"
#include "../objects/common.h"
#include "../types.h"
#include "enum.h"

#if TR_VERSION == 1
typedef struct {
    ANIM_FRAME *frame_base;
    int16_t frame_num;
    int16_t lock;
    XYZ_16 rot;
    uint16_t flash_gun;

    struct {
        struct {
            XYZ_16 rot;
        } result, prev;
    } interp;
} LARA_ARM;

typedef struct {
    int32_t ammo;
    int32_t hit;
    int32_t miss;
} AMMO_INFO;

typedef struct {
    int16_t item_num;
    int16_t gun_status;
    LARA_GUN_TYPE gun_type;
    LARA_GUN_TYPE request_gun_type;
    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;
    int16_t calc_fall_speed;
    LARA_WATER_STATE water_status;
    int16_t pose_count;
    int16_t hit_frame;
    int16_t hit_direction;
    int16_t air;
    int16_t dive_timer;
    int16_t death_timer;
    int16_t current_active;
    int32_t water_surface_dist;
    int16_t hit_effect_count;
    EFFECT *hit_effect;
    int32_t mesh_effects;
    OBJECT_MESH *mesh_ptrs[LM_NUMBER_OF];
    ITEM *target;
    int16_t target_angles[2];
    int16_t turn_rate;
    int16_t move_angle;
    XYZ_16 head_rot;
    XYZ_16 torso_rot;
    LARA_ARM left_arm;
    LARA_ARM right_arm;
    AMMO_INFO pistols;
    AMMO_INFO magnums;
    AMMO_INFO uzis;
    AMMO_INFO shotgun;
    LOT_INFO lot;
    struct {
        int32_t item_num;
        int32_t move_count;
        bool is_moving;
    } interact_target;

    struct {
        struct {
            XYZ_16 head_rot;
            XYZ_16 torso_rot;
        } result, prev;
    } interp;
} LARA_INFO;

#elif TR_VERSION == 2
typedef struct {
    int32_t ammo;
} AMMO_INFO;

typedef struct {
    ANIM_FRAME *frame_base;
    int16_t frame_num;
    int16_t anim_num;
    int16_t lock;
    XYZ_16 rot;
    int16_t flash_gun;

    struct {
        struct {
            XYZ_16 rot;
        } result, prev;
    } interp;
} LARA_ARM;

typedef struct {
    int16_t item_num;
    int16_t gun_status;
    int16_t gun_type;
    int16_t request_gun_type;
    int16_t last_gun_type;
    int16_t calc_fall_speed;
    int16_t water_status;
    int16_t climb_status;
    int16_t pose_count;
    int16_t hit_frame;
    int16_t hit_direction;
    int16_t air;
    int16_t dive_count;
    int16_t death_timer;
    int16_t current_active;
    int16_t hit_effect_count;
    int16_t flare_age;
    int16_t skidoo;
    int16_t weapon_item;
    int16_t back_gun;
    int16_t flare_frame;
    // clang-format off
    uint16_t flare_control_left:  1; // 0x01 1
    uint16_t flare_control_right: 1; // 0x02 2
    uint16_t extra_anim:          1; // 0x04 4
    uint16_t look:                1; // 0x08 8
    uint16_t burn:                1; // 0x10 16
    uint16_t pad:                 11;
    // clang-format on
    int32_t water_surface_dist;
    XYZ_32 last_pos;
    EFFECT *hit_effect;
    uint32_t mesh_effects;
    OBJECT_MESH *mesh_ptrs[LM_NUMBER_OF];
    ITEM *target;
    int16_t target_angles[2];
    int16_t turn_rate;
    int16_t move_angle;
    XYZ_16 head_rot;
    XYZ_16 torso_rot;
    LARA_ARM left_arm;
    LARA_ARM right_arm;
    AMMO_INFO pistol_ammo;
    AMMO_INFO magnum_ammo;
    AMMO_INFO uzi_ammo;
    AMMO_INFO shotgun_ammo;
    AMMO_INFO harpoon_ammo;
    AMMO_INFO grenade_ammo;
    AMMO_INFO m16_ammo;
    CREATURE *creature;

    struct {
        struct {
            XYZ_16 head_rot;
            XYZ_16 torso_rot;
        } result, prev;
    } interp;
} LARA_INFO;

#endif

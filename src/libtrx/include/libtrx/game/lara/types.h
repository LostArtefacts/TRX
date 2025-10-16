#pragma once

#include "../anims.h"
#include "../creature.h"
#include "../effects/types.h"
#include "../items/types.h"
#include "../objects/common.h"
#include "../types.h"
#include "enum.h"

typedef struct {
    ANIM_FRAME *frame_base;
    int16_t frame_num;
#if TR_VERSION >= 2
    int16_t anim_num;
#endif
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
    int32_t ammo;
} AMMO_INFO;

typedef struct {
    int16_t item_num;
    LARA_GUN_STATE gun_status;
    LARA_GUN_TYPE gun_type;
    LARA_GUN_TYPE request_gun_type;
    LARA_GUN_TYPE last_gun_type;

    LARA_WATER_STATE water_status;
    int32_t water_surface_dist;
    int16_t turn_rate;
    int16_t move_angle;
    XYZ_16 head_rot;
    XYZ_16 torso_rot;
    int16_t calc_fall_speed;
    int16_t pose_count;
    int16_t hit_frame;
    int16_t hit_direction;
    int16_t air;
    int16_t dive_timer;
    int16_t death_timer;
    int16_t sprint_timer;
    int16_t exposure_timer;
    int32_t idle_timer;
    int16_t current_active;
    LOT_INFO lot;
    XYZ_32 last_pos;

    int16_t hit_effect_count;
    EFFECT *hit_effect;
    int32_t mesh_effects;
    OBJECT_MESH *mesh_ptrs[LM_NUMBER_OF];

    ITEM *target;
    int16_t target_angles[2];

    LARA_ARM left_arm;
    LARA_ARM right_arm;
    AMMO_INFO pistol_ammo;
    AMMO_INFO magnum_ammo;
    AMMO_INFO uzi_ammo;
    AMMO_INFO shotgun_ammo;

    struct {
        struct {
            XYZ_16 head_rot;
            XYZ_16 torso_rot;
        } result, prev;
    } interp;

    bool extra_anim;
    bool burn;
    bool climb_status;
    bool killed_loyal_item;

    struct {
        int32_t item_num;
        int32_t move_count;
        bool is_moving;
        XYZ_32 initial_pos;
    } interact_target;

    LARA_GUN_TYPE holsters_gun_type;
    LARA_GUN_TYPE back_gun_type;
#if TR_VERSION >= 2
    OBJECT_ID back_gun_obj_id;
    int16_t gun_item_num;

    AMMO_INFO harpoon_ammo;
    AMMO_INFO grenade_ammo;
    AMMO_INFO m16_ammo;

    struct {
        bool control;
        int16_t age;
        int16_t frame_num;
    } flare;
#endif
} LARA_INFO;

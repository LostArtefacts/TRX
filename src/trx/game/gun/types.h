#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>
#include <trx/game/lara/enum.h>
#include <trx/game/sound/ids.h>

typedef enum {
    WEAPON_TYPE_DUAL_PISTOLS,
    WEAPON_TYPE_SINGLE_PISTOL,
    WEAPON_TYPE_RIFLE,
    WEAPON_TYPE_MOUNTED,
    NUM_WEAPON_TYPES,
} WEAPON_TYPE;

// A shot is one pull of the trigger, which for the shotgun spends six rounds.
// The flare counts a flare where a weapon counts a shot.
typedef struct {
    // What the weapon arrives with the first time Lara picks it up.
    int32_t initial_shots;
    // What one box of ammunition is worth.
    int32_t box_shots;
    // What a box shows on its inventory icon, which follows nothing else.
    int32_t box_label_qty;
    // Whether firing spends nothing, so that the weapon never runs out and
    // carries no counter.
    bool infinite;
} WEAPON_AMMO_INFO;

// How far off straight ahead an aim may go. Yaw counts to either side, pitch up
// and down.
typedef struct {
    int16_t min_yaw;
    int16_t max_yaw;
    int16_t min_pitch;
    int16_t max_pitch;
} WEAPON_AIM_LIMITS;

// An offset in the frame of the hand that holds the weapon. A weapon held in
// one hand only uses the right.
typedef struct {
    XYZ_32 right;
    XYZ_32 left;
} WEAPON_HAND_POS;

typedef struct {
    int16_t time;
    int16_t shade;
    RGB_F color;
    WEAPON_HAND_POS pos;
} WEAPON_FLASH_INFO;

typedef struct {
    RGB_F color;
    XYZ_32 pos;
    // Multiplies the glow sprite's own size; 0 disables the glow.
    float scale;
    // Randomizes the glow's brightness every frame, like a burning flare.
    bool flicker;
} WEAPON_GLOW_INFO;

// Frames of the animation Lara plays while holding the weapon, at which the
// weapon changes hands or kicks.
typedef struct {
    int16_t equip_anim_idx;
    int16_t draw_frame;
    int16_t undraw_frame;
    int16_t recoil_frame;
} WEAPON_ANIM_INFO;

typedef struct {
    LARA_GUN_TYPE gun_type;
    // Whether a module implements this weapon. A gun type that none declares
    // is one the engine carries but nothing drives, such as empty hands or a
    // gun fixed to a vehicle.
    bool is_declared;
    // Whether Lara returns to it after she puts away what she holds now.
    bool is_remembered;
    // Whether drawing it swings the camera to her back.
    bool wants_combat_camera;
    void (*draw_func)(LARA_GUN_TYPE gun_type);
    void (*undraw_func)(LARA_GUN_TYPE gun_type);
    // Puts what Lara carries into her hands, once the level holds the meshes
    // it is made of.
    void (*draw_meshes_func)(LARA_GUN_TYPE gun_type);
    // Runs each frame the weapon is live, which is not the same phase for
    // every one: a gun acts once it is ready, while a flare acts while Lara
    // is otherwise unarmed. The phase says which one runs.
    void (*control_func)(LARA_GUN_TYPE gun_type, LARA_GUN_STATE gun_status);
    WEAPON_TYPE type;
    // Where auto-aim may lock on, and how far each arm may follow it.
    WEAPON_AIM_LIMITS lock;
    WEAPON_AIM_LIMITS left_arm;
    WEAPON_AIM_LIMITS right_arm;
    int16_t aim_speed;
    int16_t shot_accuracy;
    int32_t gun_height;
    int32_t damage;
    WEAPON_AMMO_INFO ammo;
    int32_t target_dist;
    WEAPON_ANIM_INFO anim;
    WEAPON_FLASH_INFO flash;
    SAMPLE_TRX_ID sample_num;
    WEAPON_GLOW_INFO glow;
    WEAPON_HAND_POS muzzle_pos;
    WEAPON_HAND_POS shell_pos;
    int32_t smoke_count;
    bool is_available;
} WEAPON_INFO;

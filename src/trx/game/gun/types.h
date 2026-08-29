#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>
#include <trx/game/input/enum.h>
#include <trx/game/lara/enum.h>
#include <trx/game/objects/ids.h>
#include <trx/game/sound/ids.h>

typedef enum {
    WEAPON_TYPE_DUAL_PISTOLS,
    WEAPON_TYPE_SINGLE_PISTOL,
    WEAPON_TYPE_RIFLE,
    WEAPON_TYPE_MOUNTED,
    WEAPON_TYPE_FLARE,
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
    // What one pull of the trigger spends, which is one round unless the
    // weapon fires more at once, as the shotgun does.
    int32_t rounds_per_shot;
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
    // The frame a spent shell leaves the weapon at. A weapon that names no
    // frame drops its shells as it fires instead.
    int16_t shell_frame;
} WEAPON_ANIM_INFO;

// Where a weapon rides while Lara has something else in her hands.
typedef enum {
    STOW_PLACE_NONE,
    STOW_PLACE_HOLSTER,
    STOW_PLACE_BACK,
} STOW_PLACE;

// The muzzle flash one shot shows: the object it is drawn from, and how that
// object is turned at the barrel.
typedef struct {
    OBJECT_ID object_id;
    XYZ_16 rot;
} GUN_FLASH;

typedef struct {
    LARA_GUN_TYPE gun_type;
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
    SAMPLE_ID sample_num;
    SAMPLE_ID sample_overlay_num;
    int32_t sample_overlay_pitch;
    WEAPON_GLOW_INFO glow;
    WEAPON_HAND_POS muzzle_pos;
    WEAPON_HAND_POS shell_pos;
    // Where smoke leaves the weapon, which is the muzzle unless the weapon
    // gives another place, and the far end of the barrel. A weapon that
    // names a barrel end drives its smoke along the barrel and throws
    // sparks with it; one that does not lets the smoke drift.
    WEAPON_HAND_POS smoke_pos;
    WEAPON_HAND_POS smoke_tip;
    // What the weapon throws out as it fires, which is an ordinary shell
    // where the weapon names nothing.
    OBJECT_ID shell_object_id;
    // Whether spent shells leave ahead of Lara rather than fall from the
    // hand that fired, and at what angle they go.
    bool shell_throws_forward;
    int16_t shell_angle;
    // A shell slower than this carries the amount again, so that a weapon
    // that throws them far does not drop one at Lara's feet.
    int16_t shell_min_speed;
    // Whether the weapon comes down as soon as Lara stops firing, rather
    // than staying up until she puts it away.
    bool unaims_on_release;
    // Whether its flash lights the room around her.
    bool flash_lights_room;
    // Whether the player may turn its flash off, which the shotgun flash
    // setting governs.
    bool flash_is_optional;
    int32_t smoke_count;
    // The objects the weapon is made of: what Lara picks up, what its
    // ammunition arrives as, and the animations she holds it with.
    OBJECT_ID gun_object_id;
    OBJECT_ID ammo_object_id;
    OBJECT_ID anim_object_id;
    // Where the weapon rides when it is put away, and which weapon shows
    // there when Lara carries several: the lowest order wins.
    STOW_PLACE stow_place;
    int32_t stow_order;
    // The key that draws this weapon straight away, where one is bound to
    // it. A weapon that names no key is reached through the inventory only.
    INPUT_ROLE equip_input_role;
    bool is_available;

    // What the weapon does. A weapon declares these where it is
    // implemented, and the weapon data does not carry them.

    // Whether a module implements this weapon. A row that none declares is
    // a gun type the engine carries but nothing drives, such as empty hands
    // or a gun fixed to a vehicle.
    bool is_declared;

    // Whether Lara starts the game with it, and reaches for it when what
    // she holds runs dry.
    bool is_default;
    // Whether Lara returns to it after she puts away what she holds now.
    bool is_remembered;
    // Whether drawing it swings the camera to her back.
    bool wants_combat_camera;
    // Whether it keeps firing while the trigger is held, which also lets
    // Lara fire it on the move.
    bool is_machine_gun;
    // Whether Lara may bring it out under water.
    bool is_usable_underwater;
    // Whether it throws an explosive that flies on its own, rather than
    // sending a round straight at what Lara aims at.
    bool is_launcher;
    // The projectile the weapon sends on its way, which is NO_OBJECT where
    // it sends a round that arrives at once instead.
    OBJECT_ID projectile_object_id;
    void (*draw_func)(LARA_GUN_TYPE gun_type);
    void (*undraw_func)(LARA_GUN_TYPE gun_type);
    // Puts what Lara carries into her hands, once the level holds the meshes
    // it is made of.
    void (*draw_meshes_func)(LARA_GUN_TYPE gun_type);
    // Runs each frame the weapon is live, which is not the same phase for
    // every one: a gun acts once it is ready, while a flare acts while Lara
    // is otherwise unarmed. The phase says which one runs.
    void (*control_func)(LARA_GUN_TYPE gun_type, LARA_GUN_STATE gun_status);
    // Whether it sounds every shot as it fires, alternating between its
    // sample and the one beside it, as a weapon that empties fast does.
    bool has_alternating_fire_sound;
    // Returns the animation the weapon rests in once it is out, which is
    // the aim unless the weapon names a routine of its own.
    int16_t (*ready_anim_func)(void);
    // Plays the sound the weapon makes while the trigger is held, and again
    // as it stops. A weapon that names no routine falls silent between the
    // shots its own sample marks.
    void (*rapid_fire_sound_func)(bool stopping);
    // Returns the muzzle flash one shot shows. A weapon that names no
    // routine shows the ordinary flash, upright at the barrel.
    GUN_FLASH (*flash_func)(void);
    // Returns how big the smoke one shot leaves is. A weapon that names no
    // routine smokes as an ordinary round does.
    uint8_t (*smoke_size_func)(void);
    // Sends a shot on its way. Whether Lara may fire while she runs is the
    // weapon's own business, so the routine is given her state and decides.
    void (*fire_func)(LARA_GUN_TYPE gun_type, bool running);
    // The icon the ammunition counter shows beside the number of shots
    // left, where the weapon has one of its own.
    const char *ammo_icon;
    // The names a savegame gives the weapon: the rounds Lara carries now,
    // and what a level keeps of it for her return. A weapon that names none
    // is left out of the file. A required name is one the first savegame
    // format already held, so a file that lacks it is broken.
    const char *save_ammo_key;
    const char *save_resume_has_key;
    const char *save_resume_ammo_key;
    bool save_keys_required;
    // What the cheats hand out, where a weapon takes part in them. The two
    // cheats are generous by different amounts, and a weapon that names
    // neither is left out of both.
    int32_t cheat_ammo;
    int32_t cheat_key_ammo;
} WEAPON_INFO;

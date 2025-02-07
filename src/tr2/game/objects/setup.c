#include "game/lara/control.h"
#include "game/lara/hair.h"
#include "game/objects/common.h"
#include "game/objects/creatures/bandit_1.h"
#include "game/objects/creatures/bandit_2.h"
#include "game/objects/creatures/barracuda.h"
#include "game/objects/creatures/bartoli.h"
#include "game/objects/creatures/big_eel.h"
#include "game/objects/creatures/big_spider.h"
#include "game/objects/creatures/bird.h"
#include "game/objects/creatures/bird_guardian.h"
#include "game/objects/creatures/cultist_1.h"
#include "game/objects/creatures/cultist_2.h"
#include "game/objects/creatures/cultist_3.h"
#include "game/objects/creatures/diver.h"
#include "game/objects/creatures/dog.h"
#include "game/objects/creatures/dragon.h"
#include "game/objects/creatures/eel.h"
#include "game/objects/creatures/jelly.h"
#include "game/objects/creatures/monk.h"
#include "game/objects/creatures/mouse.h"
#include "game/objects/creatures/shark.h"
#include "game/objects/creatures/skidoo_driver.h"
#include "game/objects/creatures/spider.h"
#include "game/objects/creatures/tiger.h"
#include "game/objects/creatures/trex.h"
#include "game/objects/creatures/winston.h"
#include "game/objects/creatures/worker_1.h"
#include "game/objects/creatures/worker_2.h"
#include "game/objects/creatures/worker_3.h"
#include "game/objects/creatures/xian_knight.h"
#include "game/objects/creatures/xian_spearman.h"
#include "game/objects/creatures/yeti.h"
#include "game/objects/effects/blood.h"
#include "game/objects/effects/body_part.h"
#include "game/objects/effects/bubble.h"
#include "game/objects/effects/dart_effect.h"
#include "game/objects/effects/ember.h"
#include "game/objects/effects/explosion.h"
#include "game/objects/effects/flame.h"
#include "game/objects/effects/glow.h"
#include "game/objects/effects/gun_flash.h"
#include "game/objects/effects/hot_liquid.h"
#include "game/objects/effects/missile_flame.h"
#include "game/objects/effects/missile_harpoon.h"
#include "game/objects/effects/missile_knife.h"
#include "game/objects/effects/ricochet.h"
#include "game/objects/effects/snow_sprite.h"
#include "game/objects/effects/splash.h"
#include "game/objects/effects/twinkle.h"
#include "game/objects/effects/water_sprite.h"
#include "game/objects/general/alarm_sound.h"
#include "game/objects/general/bell.h"
#include "game/objects/general/big_bowl.h"
#include "game/objects/general/bird_tweeter.h"
#include "game/objects/general/bridge_flat.h"
#include "game/objects/general/bridge_tilt_1.h"
#include "game/objects/general/bridge_tilt_2.h"
#include "game/objects/general/camera_target.h"
#include "game/objects/general/clock_chimes.h"
#include "game/objects/general/copter.h"
#include "game/objects/general/cutscene_player.h"
#include "game/objects/general/detonator.h"
#include "game/objects/general/ding_dong.h"
#include "game/objects/general/door.h"
#include "game/objects/general/drawbridge.h"
#include "game/objects/general/earthquake.h"
#include "game/objects/general/final_cutscene.h"
#include "game/objects/general/final_level_counter.h"
#include "game/objects/general/flare_item.h"
#include "game/objects/general/general.h"
#include "game/objects/general/gong_bonger.h"
#include "game/objects/general/grenade.h"
#include "game/objects/general/harpoon_bolt.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/lara_alarm.h"
#include "game/objects/general/lift.h"
#include "game/objects/general/mini_copter.h"
#include "game/objects/general/movable_block.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/puzzle_hole.h"
#include "game/objects/general/secret.h"
#include "game/objects/general/sphere_of_doom.h"
#include "game/objects/general/switch.h"
#include "game/objects/general/trapdoor.h"
#include "game/objects/general/waterfall.h"
#include "game/objects/general/window.h"
#include "game/objects/general/zipline.h"
#include "game/objects/traps/blade.h"
#include "game/objects/traps/dart.h"
#include "game/objects/traps/dart_emitter.h"
#include "game/objects/traps/dying_monk.h"
#include "game/objects/traps/ember_emitter.h"
#include "game/objects/traps/falling_block.h"
#include "game/objects/traps/falling_ceiling.h"
#include "game/objects/traps/flame_emitter.h"
#include "game/objects/traps/gondola.h"
#include "game/objects/traps/hook.h"
#include "game/objects/traps/icicle.h"
#include "game/objects/traps/killer_statue.h"
#include "game/objects/traps/mine.h"
#include "game/objects/traps/pendulum.h"
#include "game/objects/traps/power_saw.h"
#include "game/objects/traps/propeller.h"
#include "game/objects/traps/rolling_ball.h"
#include "game/objects/traps/spike_ceiling.h"
#include "game/objects/traps/spike_wall.h"
#include "game/objects/traps/spikes.h"
#include "game/objects/traps/spinning_blade.h"
#include "game/objects/traps/springboard.h"
#include "game/objects/traps/teeth_trap.h"
#include "game/objects/vehicles/boat.h"
#include "game/objects/vehicles/skidoo_armed.h"
#include "game/objects/vehicles/skidoo_fast.h"
#include "global/types.h"
#include "global/vars.h"

#define DEFAULT_RADIUS 10

static void M_SetupLara(void);
static void M_SetupLaraExtra(void);
static void M_SetupBaddyObjects(void);
static void M_SetupTrapObjects(void);
static void M_SetupGeneralObjects(void);

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise = Lara_InitialiseLoad;

    obj->shadow_size = (UNIT_SHADOW / 16) * 10;
    obj->hit_points = LARA_MAX_HITPOINTS;
    obj->draw_routine = Object_DrawDummyItem;

    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_SetupLaraExtra(void)
{
    OBJECT *const obj = Object_Get(O_LARA_EXTRA);
    obj->control = Lara_ControlExtra;
}

static void M_SetupBaddyObjects(void)
{
    M_SetupLara();
    M_SetupLaraExtra();

    Dog_Setup();
    Mouse_Setup();
    Cultist1_Setup();
    Cultist1A_Setup();
    Cultist1B_Setup();
    Cultist2_Setup();
    Cultist3_Setup();
    Shark_Setup();
    Tiger_Setup();
    Barracuda_Setup();
    Spider_Setup();
    BigSpider_Setup();
    Yeti_Setup();
    Jelly_Setup();
    Diver_Setup();
    Worker1_Setup();
    Worker2_Setup();
    Worker3_Setup();
    Worker4_Setup();
    Worker5_Setup();
    Monk1_Setup();
    Monk2_Setup();
    Bird_SetupEagle();
    Bird_SetupCrow();
    SkidooDriver_Setup();
    Bartoli_Setup();
    Dragon_SetupFront();
    Dragon_SetupBack();
    Bandit1_Setup();
    Bandit2_Setup();
    Bandit2B_Setup();
    BigEel_Setup();
    Eel_Setup();
    XianKnight_Setup();
    XianSpearman_Setup();
    BirdGuardian_Setup();
    TRex_Setup();
    Winston_Setup();
}

static void M_SetupTrapObjects(void)
{
    Blade_Setup();
    DartEmitter_Setup();
    Dart_Setup();
    DyingMonk_Setup();
    EmberEmitter_Setup();
    FallingBlock_Setup(Object_Get(O_FALLING_BLOCK_1));
    FallingBlock_Setup(Object_Get(O_FALLING_BLOCK_2));
    FallingBlock_Setup(Object_Get(O_FALLING_BLOCK_3));
    FallingCeiling_Setup();
    FlameEmitter_Setup();
    General_Setup();
    Gondola_Setup();
    Hook_Setup();
    Icicle_Setup();
    KillerStatue_Setup();
    Mine_Setup();
    Pendulum_Setup(Object_Get(O_PENDULUM_1));
    Pendulum_Setup(Object_Get(O_PENDULUM_2));
    PowerSaw_Setup();
    Propeller_Setup(Object_Get(O_PROPELLER_1));
    Propeller_Setup(Object_Get(O_PROPELLER_2));
    Propeller_Setup(Object_Get(O_PROPELLER_3));
    RollingBall_Setup(Object_Get(O_ROLLING_BALL_1));
    RollingBall_Setup(Object_Get(O_ROLLING_BALL_2));
    RollingBall_Setup(Object_Get(O_ROLLING_BALL_3));
    SpikeCeiling_Setup();
    SpikeWall_Setup();
    Spikes_Setup();
    SpinningBlade_Setup();
    Springboard_Setup();
    TeethTrap_Setup();
}

static void M_SetupGeneralObjects(void)
{
    Boat_Setup();
    SkidooArmed_Setup();
    SkidooFast_Setup();

    // misc interactive objects
    Bell_Setup();
    BigBowl_Setup();
    Detonator1_Setup();
    Detonator2_Setup();
    FlareItem_Setup();
    Lift_Setup();
    Zipline_Setup();

    // misc non-interactive objects
    AlarmSound_Setup();
    BirdTweeter_Setup(Object_Get(O_BIRD_TWEETER_1));
    BirdTweeter_Setup(Object_Get(O_BIRD_TWEETER_2));
    CameraTarget_Setup();
    ClockChimes_Setup();
    DingDong_Setup();
    Earthquake_Setup();
    FinalCutscene_Setup();
    FinalLevelCounter_Setup();
    GongBonger_Setup();
    HotLiquid_Setup();
    LaraAlarm_Setup();
    Copter_Setup();
    MiniCopter_Setup();

    // push blocks
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_1));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_2));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_3));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_4));

    // projectiles
    Grenade_Setup();
    HarpoonBolt_Setup();
    MissileFlame_Setup();
    MissileHarpoon_Setup();
    MissileSpawn_Knife_Setup();
    SphereOfDoom_Setup(Object_Get(O_SPHERE_OF_DOOM_1), true);
    SphereOfDoom_Setup(Object_Get(O_SPHERE_OF_DOOM_2), true);
    SphereOfDoom_Setup(Object_Get(O_SPHERE_OF_DOOM_3), false);

    // effects
    Blood_Setup();
    BodyPart_Setup();
    Bubble_Setup();
    Explosion_Setup();
    Glow_Setup();
    Spawn_Splash_Setup();
    Twinkle_Setup();
    GunFlash_Setup();
    Spawn_Ricochet_Setup();
    SnowSprite_Setup();
    WaterSprite_Setup();
    Waterfall_Setup();
    Flame_Setup();
    DartEffect_Setup();
    Ember_Setup();

    // geometry objects
    BridgeFlat_Setup();
    BridgeTilt1_Setup();
    BridgeTilt2_Setup();
    Drawbridge_Setup();
    Window_1_Setup();
    Window_2_Setup();

    // doors
    Door_Setup(Object_Get(O_DOOR_TYPE_1));
    Door_Setup(Object_Get(O_DOOR_TYPE_2));
    Door_Setup(Object_Get(O_DOOR_TYPE_3));
    Door_Setup(Object_Get(O_DOOR_TYPE_4));
    Door_Setup(Object_Get(O_DOOR_TYPE_5));
    Door_Setup(Object_Get(O_DOOR_TYPE_6));
    Door_Setup(Object_Get(O_DOOR_TYPE_7));
    Door_Setup(Object_Get(O_DOOR_TYPE_8));
    Trapdoor_Setup(Object_Get(O_TRAPDOOR_TYPE_1));
    Trapdoor_Setup(Object_Get(O_TRAPDOOR_TYPE_2));

    // keys and puzzles
    Keyhole_Setup(Object_Get(O_KEY_HOLE_1));
    Keyhole_Setup(Object_Get(O_KEY_HOLE_2));
    Keyhole_Setup(Object_Get(O_KEY_HOLE_3));
    Keyhole_Setup(Object_Get(O_KEY_HOLE_4));
    PuzzleHole_Setup(Object_Get(O_PUZZLE_DONE_1), true);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_DONE_2), true);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_DONE_3), true);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_DONE_4), true);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_1), false);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_2), false);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_3), false);
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_4), false);

    // switches
    Switch_Setup(Object_Get(O_SWITCH_TYPE_AIRLOCK), false);
    Switch_Setup(Object_Get(O_SWITCH_TYPE_BUTTON), false);
    Switch_Setup(Object_Get(O_SWITCH_TYPE_NORMAL), false);
    Switch_Setup(Object_Get(O_SWITCH_TYPE_SMALL), false);
    Switch_Setup(Object_Get(O_SWITCH_TYPE_UW), true);

    // cutscene players
    CutscenePlayer_Setup(Object_Get(O_PLAYER_1));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_2));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_3));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_4));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_5));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_6));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_7));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_8));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_9));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_10));

    // pickups
    Pickup_Setup(Object_Get(O_FLARES_ITEM));
    Pickup_Setup(Object_Get(O_GRENADE_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_GRENADE_ITEM));
    Pickup_Setup(Object_Get(O_HARPOON_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_HARPOON_ITEM));
    Pickup_Setup(Object_Get(O_KEY_ITEM_1));
    Pickup_Setup(Object_Get(O_KEY_ITEM_2));
    Pickup_Setup(Object_Get(O_KEY_ITEM_3));
    Pickup_Setup(Object_Get(O_KEY_ITEM_4));
    Pickup_Setup(Object_Get(O_LARGE_MEDIPACK_ITEM));
    Pickup_Setup(Object_Get(O_M16_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_M16_ITEM));
    Pickup_Setup(Object_Get(O_MAGNUM_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_MAGNUM_ITEM));
    Pickup_Setup(Object_Get(O_PICKUP_ITEM_1));
    Pickup_Setup(Object_Get(O_PICKUP_ITEM_2));
    Pickup_Setup(Object_Get(O_PISTOL_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_PISTOL_ITEM));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_1));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_2));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_3));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_4));
    Pickup_Setup(Object_Get(O_SECRET_1));
    Pickup_Setup(Object_Get(O_SECRET_3));
    Pickup_Setup(Object_Get(O_SHOTGUN_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_SHOTGUN_ITEM));
    Pickup_Setup(Object_Get(O_SMALL_MEDIPACK_ITEM));
    Pickup_Setup(Object_Get(O_UZI_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_UZI_ITEM));

    Secret2_Setup();
}

void Object_SetupAllObjects(void)
{
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const obj = Object_Get(i);
        obj->initialise = nullptr;
        obj->control = nullptr;
        obj->floor = nullptr;
        obj->ceiling = nullptr;
        obj->draw_routine = Object_DrawAnimatingItem;
        obj->collision = nullptr;
        obj->hit_points = DONT_TARGET;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;

        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->intelligent = 0;
    }

    M_SetupBaddyObjects();
    M_SetupTrapObjects();
    M_SetupGeneralObjects();

    Lara_Hair_Initialise();
}

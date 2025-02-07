#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/objects/common.h"
#include "game/objects/creatures/ape.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/objects/creatures/baldy.h"
#include "game/objects/creatures/bat.h"
#include "game/objects/creatures/bear.h"
#include "game/objects/creatures/centaur.h"
#include "game/objects/creatures/cowboy.h"
#include "game/objects/creatures/crocodile.h"
#include "game/objects/creatures/cutscene_player.h"
#include "game/objects/creatures/larson.h"
#include "game/objects/creatures/lion.h"
#include "game/objects/creatures/mummy.h"
#include "game/objects/creatures/mutant.h"
#include "game/objects/creatures/natla.h"
#include "game/objects/creatures/pierre.h"
#include "game/objects/creatures/pod.h"
#include "game/objects/creatures/raptor.h"
#include "game/objects/creatures/rat.h"
#include "game/objects/creatures/skate_kid.h"
#include "game/objects/creatures/statue.h"
#include "game/objects/creatures/torso.h"
#include "game/objects/creatures/trex.h"
#include "game/objects/creatures/wolf.h"
#include "game/objects/effects/blood.h"
#include "game/objects/effects/body_part.h"
#include "game/objects/effects/bubble.h"
#include "game/objects/effects/dart_effect.h"
#include "game/objects/effects/ember.h"
#include "game/objects/effects/explosion.h"
#include "game/objects/effects/flame.h"
#include "game/objects/effects/gunshot.h"
#include "game/objects/effects/missile.h"
#include "game/objects/effects/natla_gun.h"
#include "game/objects/effects/pickup_aid.h"
#include "game/objects/effects/ricochet.h"
#include "game/objects/effects/splash.h"
#include "game/objects/effects/twinkle.h"
#include "game/objects/general/boat.h"
#include "game/objects/general/bridge_flat.h"
#include "game/objects/general/bridge_tilt1.h"
#include "game/objects/general/bridge_tilt2.h"
#include "game/objects/general/cabin.h"
#include "game/objects/general/camera_target.h"
#include "game/objects/general/cog.h"
#include "game/objects/general/door.h"
#include "game/objects/general/drawbridge.h"
#include "game/objects/general/earthquake.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/moving_bar.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/puzzle_hole.h"
#include "game/objects/general/save_crystal.h"
#include "game/objects/general/scion1.h"
#include "game/objects/general/scion2.h"
#include "game/objects/general/scion3.h"
#include "game/objects/general/scion4.h"
#include "game/objects/general/scion_holder.h"
#include "game/objects/general/switch.h"
#include "game/objects/general/trapdoor.h"
#include "game/objects/general/waterfall.h"
#include "game/objects/traps/damocles_sword.h"
#include "game/objects/traps/dart.h"
#include "game/objects/traps/dart_emitter.h"
#include "game/objects/traps/ember_emitter.h"
#include "game/objects/traps/falling_block.h"
#include "game/objects/traps/falling_ceiling.h"
#include "game/objects/traps/flame_emitter.h"
#include "game/objects/traps/lava_wedge.h"
#include "game/objects/traps/lightning_emitter.h"
#include "game/objects/traps/midas_touch.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/pendulum.h"
#include "game/objects/traps/rolling_ball.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/objects/traps/spikes.h"
#include "game/objects/traps/teeth_trap.h"
#include "game/objects/traps/thors_hammer_handle.h"
#include "game/objects/traps/thors_hammer_head.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>

// TODO: refactor similar to TR2
static void M_SetupLara(void);
static void M_SetupLaraExtra(void);
static void M_SetupCreatures(void);
static void M_SetupTraps(void);
static void M_SetupMiscObjects(void);
static void M_DisableObject(GAME_OBJECT_ID obj_id);

static void M_SetupLara(void)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->initialise = Lara_InitialiseLoad;
    obj->draw_routine = Object_DrawDummyItem;
    obj->hit_points = g_Config.gameplay.start_lara_hitpoints;
    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

static void M_SetupLaraExtra(void)
{
    OBJECT *const obj = Object_Get(O_LARA_EXTRA);
    obj->control = Lara_ControlExtra;
}

static void M_SetupCreatures(void)
{
    M_SetupLara();
    M_SetupLaraExtra();

    BaconLara_Setup(Object_Get(O_BACON_LARA));
    Wolf_Setup(Object_Get(O_WOLF));
    Bear_Setup(Object_Get(O_BEAR));
    Bat_Setup(Object_Get(O_BAT));
    TRex_Setup(Object_Get(O_TREX));
    Raptor_Setup(Object_Get(O_RAPTOR));
    Larson_Setup(Object_Get(O_LARSON));
    Pierre_Setup(Object_Get(O_PIERRE));
    Rat_Setup(Object_Get(O_RAT));
    Vole_Setup(Object_Get(O_VOLE));
    Lion_SetupLion(Object_Get(O_LION));
    Lion_SetupLioness(Object_Get(O_LIONESS));
    Lion_SetupPuma(Object_Get(O_PUMA));
    Croc_Setup(Object_Get(O_CROCODILE));
    Alligator_Setup(Object_Get(O_ALLIGATOR));
    Ape_Setup(Object_Get(O_APE));
    Mutant_Setup(Object_Get(O_WARRIOR_1));
    Mutant_Setup2(Object_Get(O_WARRIOR_2));
    Mutant_Setup3(Object_Get(O_WARRIOR_3));
    Centaur_Setup(Object_Get(O_CENTAUR));
    Mummy_Setup(Object_Get(O_MUMMY));
    SkateKid_Setup(Object_Get(O_SKATEKID));
    Cowboy_Setup(Object_Get(O_COWBOY));
    Baldy_Setup(Object_Get(O_BALDY));
    Torso_Setup(Object_Get(O_TORSO));
    Natla_Setup(Object_Get(O_NATLA));
    Pod_Setup(Object_Get(O_PODS));
    Pod_Setup(Object_Get(O_BIG_POD));
    Statue_Setup(Object_Get(O_STATUE));
}

static void M_SetupTraps(void)
{
    FallingBlock_Setup(Object_Get(O_FALLING_BLOCK));
    Pendulum_Setup(Object_Get(O_PENDULUM));
    TeethTrap_Setup(Object_Get(O_TEETH_TRAP));
    RollingBall_Setup(Object_Get(O_ROLLING_BALL));
    Spikes_Setup(Object_Get(O_SPIKES));
    FallingCeiling_Setup(Object_Get(O_FALLING_CEILING_1));
    FallingCeiling_Setup(Object_Get(O_FALLING_CEILING_2));
    DamoclesSword_Setup(Object_Get(O_DAMOCLES_SWORD));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_1));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_2));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_3));
    MovableBlock_Setup(Object_Get(O_MOVABLE_BLOCK_4));
    SlidingPillar_Setup(Object_Get(O_SLIDING_PILLAR));
    LightningEmitter_Setup(Object_Get(O_LIGHTNING_EMITTER));
    ThorsHammerHandle_Setup(Object_Get(O_THORS_HANDLE));
    ThorsHammerHead_Setup(Object_Get(O_THORS_HEAD));
    MidasTouch_Setup(Object_Get(O_MIDAS_TOUCH));
    DartEmitter_Setup(Object_Get(O_DART_EMITTER));
    Dart_Setup(Object_Get(O_DART));
    DartEffect_Setup(Object_Get(O_DART_EFFECT));
    FlameEmitter_Setup(Object_Get(O_FLAME_EMITTER));
    Flame_Setup(Object_Get(O_FLAME));
    EmberEmitter_Setup(Object_Get(O_EMBER_EMITTER));
    Ember_Setup(Object_Get(O_EMBER));
    LavaWedge_Setup(Object_Get(O_LAVA_WEDGE));
}

static void M_SetupMiscObjects(void)
{
    CameraTarget_Setup(Object_Get(O_CAMERA_TARGET));
    BridgeFlat_Setup(Object_Get(O_BRIDGE_FLAT));
    BridgeTilt1_Setup(Object_Get(O_BRIDGE_TILT_1));
    BridgeTilt2_Setup(Object_Get(O_BRIDGE_TILT_2));
    Drawbridge_Setup(Object_Get(O_DRAWBRIDGE));
    Switch_Setup(Object_Get(O_SWITCH_TYPE_1));
    Switch_SetupUW(Object_Get(O_SWITCH_TYPE_2));
    Door_Setup(Object_Get(O_DOOR_TYPE_1));
    Door_Setup(Object_Get(O_DOOR_TYPE_2));
    Door_Setup(Object_Get(O_DOOR_TYPE_3));
    Door_Setup(Object_Get(O_DOOR_TYPE_4));
    Door_Setup(Object_Get(O_DOOR_TYPE_5));
    Door_Setup(Object_Get(O_DOOR_TYPE_6));
    Door_Setup(Object_Get(O_DOOR_TYPE_7));
    Door_Setup(Object_Get(O_DOOR_TYPE_8));
    TrapDoor_Setup(Object_Get(O_TRAPDOOR_1));
    TrapDoor_Setup(Object_Get(O_TRAPDOOR_2));
    Cog_Setup(Object_Get(O_COG_1));
    Cog_Setup(Object_Get(O_COG_2));
    Cog_Setup(Object_Get(O_COG_3));
    MovingBar_Setup(Object_Get(O_MOVING_BAR));

    Pickup_Setup(Object_Get(O_PICKUP_ITEM_1));
    Pickup_Setup(Object_Get(O_PICKUP_ITEM_2));
    Pickup_Setup(Object_Get(O_KEY_ITEM_1));
    Pickup_Setup(Object_Get(O_KEY_ITEM_2));
    Pickup_Setup(Object_Get(O_KEY_ITEM_3));
    Pickup_Setup(Object_Get(O_KEY_ITEM_4));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_1));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_2));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_3));
    Pickup_Setup(Object_Get(O_PUZZLE_ITEM_4));
    Pickup_Setup(Object_Get(O_PISTOL_ITEM));
    Pickup_Setup(Object_Get(O_SHOTGUN_ITEM));
    Pickup_Setup(Object_Get(O_MAGNUM_ITEM));
    Pickup_Setup(Object_Get(O_UZI_ITEM));
    Pickup_Setup(Object_Get(O_PISTOL_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_SG_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_MAG_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_UZI_AMMO_ITEM));
    Pickup_Setup(Object_Get(O_EXPLOSIVE_ITEM));
    Pickup_Setup(Object_Get(O_MEDI_ITEM));
    Pickup_Setup(Object_Get(O_BIGMEDI_ITEM));

    Scion1_Setup(Object_Get(O_SCION_ITEM_1));
    Scion2_Setup(Object_Get(O_SCION_ITEM_2));
    Scion3_Setup(Object_Get(O_SCION_ITEM_3));
    Scion4_Setup(Object_Get(O_SCION_ITEM_4));
    ScionHolder_Setup(Object_Get(O_SCION_HOLDER));

    Pickup_Setup(Object_Get(O_LEADBAR_ITEM));
    SaveCrystal_Setup(Object_Get(O_SAVEGAME_ITEM));
    KeyHole_Setup(Object_Get(O_KEY_HOLE_1));
    KeyHole_Setup(Object_Get(O_KEY_HOLE_2));
    KeyHole_Setup(Object_Get(O_KEY_HOLE_3));
    KeyHole_Setup(Object_Get(O_KEY_HOLE_4));

    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_1));
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_2));
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_3));
    PuzzleHole_Setup(Object_Get(O_PUZZLE_HOLE_4));
    PuzzleHole_SetupDone(Object_Get(O_PUZZLE_DONE_1));
    PuzzleHole_SetupDone(Object_Get(O_PUZZLE_DONE_2));
    PuzzleHole_SetupDone(Object_Get(O_PUZZLE_DONE_3));
    PuzzleHole_SetupDone(Object_Get(O_PUZZLE_DONE_4));

    Cabin_Setup(Object_Get(O_PORTACABIN));
    Boat_Setup(Object_Get(O_BOAT));
    Earthquake_Setup(Object_Get(O_EARTHQUAKE));

    CutscenePlayer_Setup(Object_Get(O_PLAYER_1));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_2));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_3));
    CutscenePlayer_Setup(Object_Get(O_PLAYER_4));

    Blood_Setup(Object_Get(O_BLOOD_1));
    Bubble_Setup(Object_Get(O_BUBBLES_1));
    Explosion_Setup(Object_Get(O_EXPLOSION_1));
    Ricochet_Setup(Object_Get(O_RICOCHET_1));
    Twinkle_Setup(Object_Get(O_TWINKLE));
    PickupAid_Setup(Object_Get(O_PICKUP_AID));
    Splash_Setup(Object_Get(O_SPLASH_1));
    Waterfall_Setup(Object_Get(O_WATERFALL));
    BodyPart_Setup(Object_Get(O_BODY_PART));
    NatlaGun_Setup(Object_Get(O_MISSILE_1));
    Missile_Setup(Object_Get(O_MISSILE_2));
    Missile_Setup(Object_Get(O_MISSILE_3));
    GunShot_Setup(Object_Get(O_GUN_FLASH));
}

static void M_DisableObject(const GAME_OBJECT_ID obj_id)
{
    OBJECT *const obj = Object_Get(obj_id);
    obj->initialise = nullptr;
    obj->collision = nullptr;
    obj->control = nullptr;
    obj->draw_routine = Object_DrawDummyItem;
    obj->floor_height_func = nullptr;
    obj->ceiling_height_func = nullptr;
}

void Object_SetupAllObjects(void)
{
    for (int i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const obj = Object_Get(i);
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise = nullptr;
        obj->collision = nullptr;
        obj->control = nullptr;
        obj->draw_routine = Object_DrawAnimatingItem;
        obj->ceiling_height_func = nullptr;
        obj->floor_height_func = nullptr;
        obj->is_usable = nullptr;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
    }

    M_SetupCreatures();
    M_SetupTraps();
    M_SetupMiscObjects();

    Lara_Hair_Initialise();

    if (g_Config.gameplay.disable_medpacks) {
        M_DisableObject(O_MEDI_ITEM);
        M_DisableObject(O_BIGMEDI_ITEM);
    }

    if (g_Config.gameplay.disable_magnums) {
        M_DisableObject(O_MAGNUM_ITEM);
        M_DisableObject(O_MAG_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_uzis) {
        M_DisableObject(O_UZI_ITEM);
        M_DisableObject(O_UZI_AMMO_ITEM);
    }

    if (g_Config.gameplay.disable_shotgun) {
        M_DisableObject(O_SHOTGUN_ITEM);
        M_DisableObject(O_SG_AMMO_ITEM);
    }
}

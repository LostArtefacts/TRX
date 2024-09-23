#include "game/objects/setup.h"

#include "config.h"
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

static void M_SetupCreatures(void);
static void M_SetupTraps(void);
static void M_SetupMiscObjects(void);

static void M_SetupCreatures(void)
{
    g_Objects[O_LARA].initialise = Lara_InitialiseLoad;
    g_Objects[O_LARA].draw_routine = Object_DrawDummyItem;
    g_Objects[O_LARA].hit_points = g_Config.start_lara_hitpoints;
    g_Objects[O_LARA].shadow_size = (UNIT_SHADOW * 10) / 16;
    g_Objects[O_LARA].save_position = 1;
    g_Objects[O_LARA].save_hitpoints = 1;
    g_Objects[O_LARA].save_anim = 1;
    g_Objects[O_LARA].save_flags = 1;

    g_Objects[O_LARA_EXTRA].control = Lara_ControlExtra;

    BaconLara_Setup(&g_Objects[O_BACON_LARA]);
    Wolf_Setup(&g_Objects[O_WOLF]);
    Bear_Setup(&g_Objects[O_BEAR]);
    Bat_Setup(&g_Objects[O_BAT]);
    TRex_Setup(&g_Objects[O_TREX]);
    Raptor_Setup(&g_Objects[O_RAPTOR]);
    Larson_Setup(&g_Objects[O_LARSON]);
    Pierre_Setup(&g_Objects[O_PIERRE]);
    Rat_Setup(&g_Objects[O_RAT]);
    Vole_Setup(&g_Objects[O_VOLE]);
    Lion_SetupLion(&g_Objects[O_LION]);
    Lion_SetupLioness(&g_Objects[O_LIONESS]);
    Lion_SetupPuma(&g_Objects[O_PUMA]);
    Croc_Setup(&g_Objects[O_CROCODILE]);
    Alligator_Setup(&g_Objects[O_ALLIGATOR]);
    Ape_Setup(&g_Objects[O_APE]);
    Mutant_Setup(&g_Objects[O_WARRIOR_1]);
    Mutant_Setup2(&g_Objects[O_WARRIOR_2]);
    Mutant_Setup3(&g_Objects[O_WARRIOR_3]);
    Centaur_Setup(&g_Objects[O_CENTAUR]);
    Mummy_Setup(&g_Objects[O_MUMMY]);
    SkateKid_Setup(&g_Objects[O_SKATEKID]);
    Cowboy_Setup(&g_Objects[O_COWBOY]);
    Baldy_Setup(&g_Objects[O_BALDY]);
    Torso_Setup(&g_Objects[O_TORSO]);
    Natla_Setup(&g_Objects[O_NATLA]);
    Pod_Setup(&g_Objects[O_PODS]);
    Pod_Setup(&g_Objects[O_BIG_POD]);
    Statue_Setup(&g_Objects[O_STATUE]);
}

static void M_SetupTraps(void)
{
    FallingBlock_Setup(&g_Objects[O_FALLING_BLOCK]);
    Pendulum_Setup(&g_Objects[O_PENDULUM]);
    TeethTrap_Setup(&g_Objects[O_TEETH_TRAP]);
    RollingBall_Setup(&g_Objects[O_ROLLING_BALL]);
    Spikes_Setup(&g_Objects[O_SPIKES]);
    FallingCeiling_Setup(&g_Objects[O_FALLING_CEILING_1]);
    FallingCeiling_Setup(&g_Objects[O_FALLING_CEILING_2]);
    DamoclesSword_Setup(&g_Objects[O_DAMOCLES_SWORD]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK_1]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK_2]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK_3]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK_4]);
    SlidingPillar_Setup(&g_Objects[O_SLIDING_PILLAR]);
    LightningEmitter_Setup(&g_Objects[O_LIGHTNING_EMITTER]);
    ThorsHammerHandle_Setup(&g_Objects[O_THORS_HANDLE]);
    ThorsHammerHead_Setup(&g_Objects[O_THORS_HEAD]);
    MidasTouch_Setup(&g_Objects[O_MIDAS_TOUCH]);
    DartEmitter_Setup(&g_Objects[O_DART_EMITTER]);
    Dart_Setup(&g_Objects[O_DARTS]);
    DartEffect_Setup(&g_Objects[O_DART_EFFECT]);
    FlameEmitter_Setup(&g_Objects[O_FLAME_EMITTER]);
    Flame_Setup(&g_Objects[O_FLAME]);
    EmberEmitter_Setup(&g_Objects[O_EMBER_EMITTER]);
    Ember_Setup(&g_Objects[O_EMBER]);
    LavaWedge_Setup(&g_Objects[O_LAVA_WEDGE]);
}

static void M_SetupMiscObjects(void)
{
    CameraTarget_Setup(&g_Objects[O_CAMERA_TARGET]);
    BridgeFlat_Setup(&g_Objects[O_BRIDGE_FLAT]);
    BridgeTilt1_Setup(&g_Objects[O_BRIDGE_TILT_1]);
    BridgeTilt2_Setup(&g_Objects[O_BRIDGE_TILT_2]);
    Drawbridge_Setup(&g_Objects[O_DRAWBRIDGE]);
    Switch_Setup(&g_Objects[O_SWITCH_TYPE_1]);
    Switch_SetupUW(&g_Objects[O_SWITCH_TYPE_2]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_1]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_2]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_3]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_4]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_5]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_6]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_7]);
    Door_Setup(&g_Objects[O_DOOR_TYPE_8]);
    TrapDoor_Setup(&g_Objects[O_TRAPDOOR_1]);
    TrapDoor_Setup(&g_Objects[O_TRAPDOOR_2]);
    Cog_Setup(&g_Objects[O_COG_1]);
    Cog_Setup(&g_Objects[O_COG_2]);
    Cog_Setup(&g_Objects[O_COG_3]);
    MovingBar_Setup(&g_Objects[O_MOVING_BAR]);

    Pickup_Setup(&g_Objects[O_PICKUP_ITEM_1]);
    Pickup_Setup(&g_Objects[O_PICKUP_ITEM_2]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM_1]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM_2]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM_3]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM_4]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM_1]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM_2]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM_3]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM_4]);
    Pickup_Setup(&g_Objects[O_PISTOL_ITEM]);
    Pickup_Setup(&g_Objects[O_SHOTGUN_ITEM]);
    Pickup_Setup(&g_Objects[O_MAGNUM_ITEM]);
    Pickup_Setup(&g_Objects[O_UZI_ITEM]);
    Pickup_Setup(&g_Objects[O_PISTOL_AMMO_ITEM]);
    Pickup_Setup(&g_Objects[O_SG_AMMO_ITEM]);
    Pickup_Setup(&g_Objects[O_MAG_AMMO_ITEM]);
    Pickup_Setup(&g_Objects[O_UZI_AMMO_ITEM]);
    Pickup_Setup(&g_Objects[O_EXPLOSIVE_ITEM]);
    Pickup_Setup(&g_Objects[O_MEDI_ITEM]);
    Pickup_Setup(&g_Objects[O_BIGMEDI_ITEM]);

    Scion1_Setup(&g_Objects[O_SCION_ITEM_1]);
    Scion2_Setup(&g_Objects[O_SCION_ITEM_2]);
    Scion3_Setup(&g_Objects[O_SCION_ITEM_3]);
    Scion4_Setup(&g_Objects[O_SCION_ITEM_4]);
    ScionHolder_Setup(&g_Objects[O_SCION_HOLDER]);

    Pickup_Setup(&g_Objects[O_LEADBAR_ITEM]);
    SaveCrystal_Setup(&g_Objects[O_SAVEGAME_ITEM]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE_1]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE_2]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE_3]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE_4]);

    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE_1]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE_2]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE_3]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE_4]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE_1]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE_2]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE_3]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE_4]);

    Cabin_Setup(&g_Objects[O_PORTACABIN]);
    Boat_Setup(&g_Objects[O_BOAT]);
    Earthquake_Setup(&g_Objects[O_EARTHQUAKE]);

    CutscenePlayer_Setup(&g_Objects[O_PLAYER_1]);
    CutscenePlayer_Setup(&g_Objects[O_PLAYER_2]);
    CutscenePlayer_Setup(&g_Objects[O_PLAYER_3]);
    CutscenePlayer_Setup(&g_Objects[O_PLAYER_4]);

    Blood_Setup(&g_Objects[O_BLOOD_1]);
    Bubble_Setup(&g_Objects[O_BUBBLES_1]);
    Explosion_Setup(&g_Objects[O_EXPLOSION_1]);
    Ricochet_Setup(&g_Objects[O_RICOCHET_1]);
    Twinkle_Setup(&g_Objects[O_TWINKLE]);
    Splash_Setup(&g_Objects[O_SPLASH_1]);
    Waterfall_Setup(&g_Objects[O_WATERFALL]);
    BodyPart_Setup(&g_Objects[O_BODY_PART]);
    NatlaGun_Setup(&g_Objects[O_MISSILE_1]);
    Missile_Setup(&g_Objects[O_MISSILE_2]);
    Missile_Setup(&g_Objects[O_MISSILE_3]);
    GunShot_Setup(&g_Objects[O_GUN_FLASH]);
}

void Object_SetupAllObjects(void)
{
    for (int i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *obj = &g_Objects[i];
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise = NULL;
        obj->collision = NULL;
        obj->control = NULL;
        obj->draw_routine = Object_DrawAnimatingItem;
        obj->ceiling_height_func = NULL;
        obj->floor_height_func = NULL;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
    }

    M_SetupCreatures();
    M_SetupTraps();
    M_SetupMiscObjects();

    Lara_Hair_Initialise();

    if (g_Config.disable_medpacks) {
        g_Objects[O_MEDI_ITEM].initialise = NULL;
        g_Objects[O_MEDI_ITEM].collision = NULL;
        g_Objects[O_MEDI_ITEM].control = NULL;
        g_Objects[O_MEDI_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_MEDI_ITEM].floor_height_func = NULL;
        g_Objects[O_MEDI_ITEM].ceiling_height_func = NULL;

        g_Objects[O_BIGMEDI_ITEM].initialise = NULL;
        g_Objects[O_BIGMEDI_ITEM].collision = NULL;
        g_Objects[O_BIGMEDI_ITEM].control = NULL;
        g_Objects[O_BIGMEDI_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_BIGMEDI_ITEM].floor_height_func = NULL;
        g_Objects[O_BIGMEDI_ITEM].ceiling_height_func = NULL;
    }

    if (g_Config.disable_magnums) {
        g_Objects[O_MAGNUM_ITEM].initialise = NULL;
        g_Objects[O_MAGNUM_ITEM].collision = NULL;
        g_Objects[O_MAGNUM_ITEM].control = NULL;
        g_Objects[O_MAGNUM_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_MAGNUM_ITEM].floor_height_func = NULL;
        g_Objects[O_MAGNUM_ITEM].ceiling_height_func = NULL;

        g_Objects[O_MAG_AMMO_ITEM].initialise = NULL;
        g_Objects[O_MAG_AMMO_ITEM].collision = NULL;
        g_Objects[O_MAG_AMMO_ITEM].control = NULL;
        g_Objects[O_MAG_AMMO_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_MAG_AMMO_ITEM].floor_height_func = NULL;
        g_Objects[O_MAG_AMMO_ITEM].ceiling_height_func = NULL;
    }

    if (g_Config.disable_uzis) {
        g_Objects[O_UZI_ITEM].initialise = NULL;
        g_Objects[O_UZI_ITEM].collision = NULL;
        g_Objects[O_UZI_ITEM].control = NULL;
        g_Objects[O_UZI_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_UZI_ITEM].floor_height_func = NULL;
        g_Objects[O_UZI_ITEM].ceiling_height_func = NULL;

        g_Objects[O_UZI_AMMO_ITEM].initialise = NULL;
        g_Objects[O_UZI_AMMO_ITEM].collision = NULL;
        g_Objects[O_UZI_AMMO_ITEM].control = NULL;
        g_Objects[O_UZI_AMMO_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_UZI_AMMO_ITEM].floor_height_func = NULL;
        g_Objects[O_UZI_AMMO_ITEM].ceiling_height_func = NULL;
    }

    if (g_Config.disable_shotgun) {
        g_Objects[O_SHOTGUN_ITEM].initialise = NULL;
        g_Objects[O_SHOTGUN_ITEM].collision = NULL;
        g_Objects[O_SHOTGUN_ITEM].control = NULL;
        g_Objects[O_SHOTGUN_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_SHOTGUN_ITEM].floor_height_func = NULL;
        g_Objects[O_SHOTGUN_ITEM].ceiling_height_func = NULL;

        g_Objects[O_SG_AMMO_ITEM].initialise = NULL;
        g_Objects[O_SG_AMMO_ITEM].collision = NULL;
        g_Objects[O_SG_AMMO_ITEM].control = NULL;
        g_Objects[O_SG_AMMO_ITEM].draw_routine = Object_DrawDummyItem;
        g_Objects[O_SG_AMMO_ITEM].floor_height_func = NULL;
        g_Objects[O_SG_AMMO_ITEM].ceiling_height_func = NULL;
    }
}

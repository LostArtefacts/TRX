#include "game/setup.h"

#include "config.h"
#include "game/lara.h"
#include "game/lara/lara_hair.h"
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
#include "game/objects/effects/earthquake.h"
#include "game/objects/effects/explosion.h"
#include "game/objects/effects/gunshot.h"
#include "game/objects/effects/missile.h"
#include "game/objects/effects/ricochet.h"
#include "game/objects/effects/splash.h"
#include "game/objects/effects/twinkle.h"
#include "game/objects/effects/waterfall.h"
#include "game/objects/general/boat.h"
#include "game/objects/general/bridge.h"
#include "game/objects/general/cabin.h"
#include "game/objects/general/cog.h"
#include "game/objects/general/door.h"
#include "game/objects/general/keyhole.h"
#include "game/objects/general/misc.h"
#include "game/objects/general/pickup.h"
#include "game/objects/general/puzzle_hole.h"
#include "game/objects/general/save_crystal.h"
#include "game/objects/general/scion.h"
#include "game/objects/general/switch.h"
#include "game/objects/general/trapdoor.h"
#include "game/objects/traps/damocles_sword.h"
#include "game/objects/traps/dart.h"
#include "game/objects/traps/falling_block.h"
#include "game/objects/traps/falling_ceiling.h"
#include "game/objects/traps/flame.h"
#include "game/objects/traps/lava.h"
#include "game/objects/traps/lightning_emitter.h"
#include "game/objects/traps/midas_touch.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/pendulum.h"
#include "game/objects/traps/rolling_ball.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/objects/traps/spikes.h"
#include "game/objects/traps/teeth_trap.h"
#include "game/objects/traps/thors_hammer.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stddef.h>

void Setup_Creatures(void)
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
    Mutant_Setup(&g_Objects[O_WARRIOR1]);
    Mutant_Setup2(&g_Objects[O_WARRIOR2]);
    Mutant_Setup3(&g_Objects[O_WARRIOR3]);
    Centaur_Setup(&g_Objects[O_CENTAUR]);
    Mummy_Setup(&g_Objects[O_MUMMY]);
    SkateKid_Setup(&g_Objects[O_SKATEKID]);
    Cowboy_Setup(&g_Objects[O_COWBOY]);
    Baldy_Setup(&g_Objects[O_BALDY]);
    Torso_Setup(&g_Objects[O_TORSO]);
    Natla_Setup(&g_Objects[O_NATLA]);
    Pod_Setup(&g_Objects[O_PODS]);
    Pod_SetupBig(&g_Objects[O_BIG_POD]);
    Statue_Setup(&g_Objects[O_STATUE]);
}

void Setup_Traps(void)
{
    FallingBlock_Setup(&g_Objects[O_FALLING_BLOCK]);
    Pendulum_Setup(&g_Objects[O_PENDULUM]);
    TeethTrap_Setup(&g_Objects[O_TEETH_TRAP]);
    RollingBall_Setup(&g_Objects[O_ROLLING_BALL]);
    Spikes_Setup(&g_Objects[O_SPIKES]);
    FallingCeiling_Setup(&g_Objects[O_FALLING_CEILING1]);
    FallingCeiling_Setup(&g_Objects[O_FALLING_CEILING2]);
    DamoclesSword_Setup(&g_Objects[O_DAMOCLES_SWORD]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK2]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK3]);
    MovableBlock_Setup(&g_Objects[O_MOVABLE_BLOCK4]);
    SlidingPillar_Setup(&g_Objects[O_SLIDING_PILLAR]);
    LightningEmitter_Setup(&g_Objects[O_LIGHTNING_EMITTER]);
    ThorsHandle_Setup(&g_Objects[O_THORS_HANDLE]);
    ThorsHead_Setup(&g_Objects[O_THORS_HEAD]);
    MidasTouch_Setup(&g_Objects[O_MIDAS_TOUCH]);
    DartEmitter_Setup(&g_Objects[O_DART_EMITTER]);
    Dart_Setup(&g_Objects[O_DARTS]);
    DartEffect_Setup(&g_Objects[O_DART_EFFECT]);
    FlameEmitter_Setup(&g_Objects[O_FLAME_EMITTER]);
    Flame_Setup(&g_Objects[O_FLAME]);
    LavaEmitter_Setup(&g_Objects[O_LAVA_EMITTER]);
    Lava_Setup(&g_Objects[O_LAVA]);
    LavaWedge_Setup(&g_Objects[O_LAVA_WEDGE]);
}

void Setup_MiscObjects(void)
{
    CameraTarget_Setup(&g_Objects[O_CAMERA_TARGET]);
    Bridge_SetupFlat(&g_Objects[O_BRIDGE_FLAT]);
    Bridge_SetupTilt1(&g_Objects[O_BRIDGE_TILT1]);
    Bridge_SetupTilt2(&g_Objects[O_BRIDGE_TILT2]);
    Bridge_SetupDrawBridge(&g_Objects[O_DRAW_BRIDGE]);
    Switch_Setup(&g_Objects[O_SWITCH_TYPE1]);
    Switch_SetupUW(&g_Objects[O_SWITCH_TYPE2]);
    Door_Setup(&g_Objects[O_DOOR_TYPE1]);
    Door_Setup(&g_Objects[O_DOOR_TYPE2]);
    Door_Setup(&g_Objects[O_DOOR_TYPE3]);
    Door_Setup(&g_Objects[O_DOOR_TYPE4]);
    Door_Setup(&g_Objects[O_DOOR_TYPE5]);
    Door_Setup(&g_Objects[O_DOOR_TYPE6]);
    Door_Setup(&g_Objects[O_DOOR_TYPE7]);
    Door_Setup(&g_Objects[O_DOOR_TYPE8]);
    TrapDoor_Setup(&g_Objects[O_TRAPDOOR]);
    TrapDoor_Setup(&g_Objects[O_TRAPDOOR2]);
    Cog_Setup(&g_Objects[O_COG_1]);
    Cog_Setup(&g_Objects[O_COG_2]);
    Cog_Setup(&g_Objects[O_COG_3]);
    MovingBar_Setup(&g_Objects[O_MOVING_BAR]);

    Pickup_Setup(&g_Objects[O_PICKUP_ITEM1]);
    Pickup_Setup(&g_Objects[O_PICKUP_ITEM2]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM1]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM2]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM3]);
    Pickup_Setup(&g_Objects[O_KEY_ITEM4]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM1]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM2]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM3]);
    Pickup_Setup(&g_Objects[O_PUZZLE_ITEM4]);
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

    Scion_Setup1(&g_Objects[O_SCION_ITEM]);
    Scion_Setup2(&g_Objects[O_SCION_ITEM2]);
    Scion_Setup3(&g_Objects[O_SCION_ITEM3]);
    Scion_Setup4(&g_Objects[O_SCION_ITEM4]);
    Scion_SetupHolder(&g_Objects[O_SCION_HOLDER]);

    LeadBar_Setup(&g_Objects[O_LEADBAR_ITEM]);
    SaveCrystal_Setup(&g_Objects[O_SAVEGAME_ITEM]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE1]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE2]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE3]);
    KeyHole_Setup(&g_Objects[O_KEY_HOLE4]);

    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE1]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE2]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE3]);
    PuzzleHole_Setup(&g_Objects[O_PUZZLE_HOLE4]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE1]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE2]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE3]);
    PuzzleHole_SetupDone(&g_Objects[O_PUZZLE_DONE4]);

    Cabin_Setup(&g_Objects[O_PORTACABIN]);
    Boat_Setup(&g_Objects[O_BOAT]);
    Earthquake_Setup(&g_Objects[O_EARTHQUAKE]);

    CutscenePlayer1_Setup(&g_Objects[O_PLAYER_1]);
    CutscenePlayer2_Setup(&g_Objects[O_PLAYER_2]);
    CutscenePlayer3_Setup(&g_Objects[O_PLAYER_3]);
    CutscenePlayer4_Setup(&g_Objects[O_PLAYER_4]);

    Blood_Setup(&g_Objects[O_BLOOD1]);
    Bubble_Setup(&g_Objects[O_BUBBLES1]);
    Explosion_Setup(&g_Objects[O_EXPLOSION1]);
    Ricochet_Setup(&g_Objects[O_RICOCHET1]);
    Twinkle_Setup(&g_Objects[O_TWINKLE]);
    Splash_Setup(&g_Objects[O_SPLASH1]);
    Waterfall_Setup(&g_Objects[O_WATERFALL]);
    BodyPart_Setup(&g_Objects[O_BODY_PART]);
    NatlaGun_Setup(&g_Objects[O_MISSILE1]);
    Missile_Setup(&g_Objects[O_MISSILE2]);
    Missile_Setup(&g_Objects[O_MISSILE3]);
    GunShot_Setup(&g_Objects[O_GUN_FLASH]);
}

void Setup_AllObjects(void)
{
    for (int i = 0; i < O_NUMBER_OF; i++) {
        OBJECT_INFO *obj = &g_Objects[i];
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

    Setup_Creatures();
    Setup_Traps();
    Setup_MiscObjects();

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

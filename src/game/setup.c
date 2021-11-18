#include "game/setup.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "game/ai/abortion.h"
#include "game/ai/alligator.h"
#include "game/ai/ape.h"
#include "game/ai/baldy.h"
#include "game/ai/bat.h"
#include "game/ai/bear.h"
#include "game/ai/centaur.h"
#include "game/ai/cowboy.h"
#include "game/ai/crocodile.h"
#include "game/ai/dino.h"
#include "game/ai/evil_lara.h"
#include "game/ai/larson.h"
#include "game/ai/lion.h"
#include "game/ai/mummy.h"
#include "game/ai/mutant.h"
#include "game/ai/natla.h"
#include "game/ai/pierre.h"
#include "game/ai/pod.h"
#include "game/ai/raptor.h"
#include "game/ai/rat.h"
#include "game/ai/skate_kid.h"
#include "game/ai/statue.h"
#include "game/ai/vole.h"
#include "game/ai/wolf.h"
#include "game/cinema.h"
#include "game/draw.h"
#include "game/effects/blood.h"
#include "game/effects/body_part.h"
#include "game/effects/bubble.h"
#include "game/effects/earthquake.h"
#include "game/effects/explosion.h"
#include "game/effects/missile.h"
#include "game/effects/ricochet.h"
#include "game/effects/splash.h"
#include "game/effects/twinkle.h"
#include "game/effects/waterfall.h"
#include "game/gamebuf.h"
#include "game/hair.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/boat.h"
#include "game/objects/bridge.h"
#include "game/objects/cabin.h"
#include "game/objects/cog.h"
#include "game/objects/door.h"
#include "game/objects/gunshot.h"
#include "game/objects/keyhole.h"
#include "game/objects/misc.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/objects/savegame_crystal.h"
#include "game/objects/scion.h"
#include "game/objects/switch.h"
#include "game/objects/trapdoor.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/traps/damocles_sword.h"
#include "game/traps/dart.h"
#include "game/traps/dart_emitter.h"
#include "game/traps/falling_block.h"
#include "game/traps/falling_ceiling.h"
#include "game/traps/flame.h"
#include "game/traps/lava.h"
#include "game/traps/lightning_emitter.h"
#include "game/traps/midas_touch.h"
#include "game/traps/movable_block.h"
#include "game/traps/pendulum.h"
#include "game/traps/rolling_ball.h"
#include "game/traps/rolling_block.h"
#include "game/traps/spikes.h"
#include "game/traps/teeth_trap.h"
#include "game/traps/thors_hammer.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"
#include "specific/s_file.h"
#include "specific/s_output.h"

#include <stddef.h>

int32_t InitialiseLevel(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    LOG_DEBUG("%d", level_num);
    if (level_type == GFL_SAVED) {
        CurrentLevel = SaveGame.current_level;
    } else {
        CurrentLevel = level_num;
    }

    Text_RemoveAll();
    InitialiseGameFlags();

    Lara.item_number = NO_ITEM;
    if (level_num != GF.title_level_num) {
        TempVideoRemove();
    }

    if (!S_LoadLevel(CurrentLevel)) {
        return 0;
    }

    if (Lara.item_number != NO_ITEM) {
        InitialiseLara();
    }

    Effects = GameBuf_Alloc(NUM_EFFECTS * sizeof(FX_INFO), GBUF_EFFECTS);
    InitialiseFXArray();
    InitialiseLOTArray();

    InitColours();
    Overlay_Init();

    HealthBarTimer = 100;
    Sound_ResetEffects();

    if (level_type == GFL_SAVED) {
        ExtractSaveGameInfo();
    }

    // LaraGun() expects request_gun_type to be set only when it really is
    // needed (see https://github.com/rr-/Tomb1Main/issues/36), not at all
    // times.
    Lara.request_gun_type = LGT_UNARMED;

    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);

    if (GF.levels[CurrentLevel].music) {
        Music_Play(GF.levels[CurrentLevel].music);
    }
    Camera.underwater = 0;
    return 1;
}

void InitialiseGameFlags()
{
    FlipStatus = 0;
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        FlipMapTable[i] = 0;
    }

    for (int i = 0; i < MAX_CD_TRACKS; i++) {
        MusicTrackFlags[i] = 0;
    }

    /* Clear Object Loaded flags */
    for (int i = 0; i < O_NUMBER_OF; i++) {
        Objects[i].loaded = 0;
    }

    LevelComplete = false;
    FlipEffect = -1;
    PierreItemNum = NO_ITEM;
}

void InitialiseLevelFlags()
{
    SaveGame.secrets = 0;
    SaveGame.timer = 0;
    SaveGame.pickups = 0;
    SaveGame.kills = 0;
}

void BaddyObjects()
{
    Objects[O_LARA].initialise = InitialiseLaraLoad;
    Objects[O_LARA].draw_routine = DrawDummyItem;
    Objects[O_LARA].hit_points = LARA_HITPOINTS;
    Objects[O_LARA].shadow_size = (UNIT_SHADOW * 10) / 16;
    Objects[O_LARA].save_position = 1;
    Objects[O_LARA].save_hitpoints = 1;
    Objects[O_LARA].save_anim = 1;
    Objects[O_LARA].save_flags = 1;

    Objects[O_LARA_EXTRA].control = ControlLaraExtra;

    SetupEvilLara(&Objects[O_EVIL_LARA]);
    SetupWolf(&Objects[O_WOLF]);
    SetupBear(&Objects[O_BEAR]);
    SetupBat(&Objects[O_BAT]);
    SetupDino(&Objects[O_DINOSAUR]);
    SetupRaptor(&Objects[O_RAPTOR]);
    SetupLarson(&Objects[O_LARSON]);
    SetupPierre(&Objects[O_PIERRE]);
    SetupRat(&Objects[O_RAT]);
    SetupVole(&Objects[O_VOLE]);
    SetupLion(&Objects[O_LION]);
    SetupLioness(&Objects[O_LIONESS]);
    SetupPuma(&Objects[O_PUMA]);
    SetupCrocodile(&Objects[O_CROCODILE]);
    SetupAlligator(&Objects[O_ALLIGATOR]);
    SetupApe(&Objects[O_APE]);
    SetupWarrior(&Objects[O_WARRIOR1]);
    SetupWarrior2(&Objects[O_WARRIOR2]);
    SetupWarrior3(&Objects[O_WARRIOR3]);
    SetupCentaur(&Objects[O_CENTAUR]);
    SetupMummy(&Objects[O_MUMMY]);
    SetupSkateKid(&Objects[O_MERCENARY1]);
    SetupCowboy(&Objects[O_MERCENARY2]);
    SetupBaldy(&Objects[O_MERCENARY3]);
    SetupAbortion(&Objects[O_ABORTION]);
    SetupNatla(&Objects[O_NATLA]);
    SetupPod(&Objects[O_PODS]);
    SetupBigPod(&Objects[O_BIG_POD]);
    SetupStatue(&Objects[O_STATUE]);
}

void TrapObjects()
{
    SetupFallingBlock(&Objects[O_FALLING_BLOCK]);
    SetupPendulum(&Objects[O_PENDULUM]);
    SetupTeethTrap(&Objects[O_TEETH_TRAP]);
    SetupRollingBall(&Objects[O_ROLLING_BALL]);
    SetupSpikes(&Objects[O_SPIKES]);
    SetupFallingCeilling(&Objects[O_FALLING_CEILING1]);
    SetupFallingCeilling(&Objects[O_FALLING_CEILING2]);
    SetupDamoclesSword(&Objects[O_DAMOCLES_SWORD]);
    SetupMovableBlock(&Objects[O_MOVABLE_BLOCK]);
    SetupMovableBlock(&Objects[O_MOVABLE_BLOCK2]);
    SetupMovableBlock(&Objects[O_MOVABLE_BLOCK3]);
    SetupMovableBlock(&Objects[O_MOVABLE_BLOCK4]);
    SetupRollingBlock(&Objects[O_ROLLING_BLOCK]);
    SetupLightningEmitter(&Objects[O_LIGHTNING_EMITTER]);
    SetupThorsHandle(&Objects[O_THORS_HANDLE]);
    SetupThorsHead(&Objects[O_THORS_HEAD]);
    SetupMidasTouch(&Objects[O_MIDAS_TOUCH]);
    SetupDartEmitter(&Objects[O_DART_EMITTER]);
    SetupDart(&Objects[O_DARTS]);
    SetupDartEffect(&Objects[O_DART_EFFECT]);
    SetupFlameEmitter(&Objects[O_FLAME_EMITTER]);
    SetupFlame(&Objects[O_FLAME]);
    SetupLavaEmitter(&Objects[O_LAVA_EMITTER]);
    SetupLava(&Objects[O_LAVA]);
    SetupLavaWedge(&Objects[O_LAVA_WEDGE]);
}

void ObjectObjects()
{
    SetupCameraTarget(&Objects[O_CAMERA_TARGET]);
    SetupBridgeFlat(&Objects[O_BRIDGE_FLAT]);
    SetupBridgeTilt1(&Objects[O_BRIDGE_TILT1]);
    SetupBridgeTilt2(&Objects[O_BRIDGE_TILT2]);
    SetupDrawBridge(&Objects[O_DRAW_BRIDGE]);
    SetupSwitch1(&Objects[O_SWITCH_TYPE1]);
    SetupSwitch2(&Objects[O_SWITCH_TYPE2]);
    SetupDoor(&Objects[O_DOOR_TYPE1]);
    SetupDoor(&Objects[O_DOOR_TYPE2]);
    SetupDoor(&Objects[O_DOOR_TYPE3]);
    SetupDoor(&Objects[O_DOOR_TYPE4]);
    SetupDoor(&Objects[O_DOOR_TYPE5]);
    SetupDoor(&Objects[O_DOOR_TYPE6]);
    SetupDoor(&Objects[O_DOOR_TYPE7]);
    SetupDoor(&Objects[O_DOOR_TYPE8]);
    SetupTrapDoor(&Objects[O_TRAPDOOR]);
    SetupTrapDoor(&Objects[O_TRAPDOOR2]);
    SetupCog(&Objects[O_COG_1]);
    SetupCog(&Objects[O_COG_2]);
    SetupCog(&Objects[O_COG_3]);
    SetupMovingBar(&Objects[O_MOVING_BAR]);

    SetupPickupObject(&Objects[O_PICKUP_ITEM1]);
    SetupPickupObject(&Objects[O_PICKUP_ITEM2]);
    SetupPickupObject(&Objects[O_KEY_ITEM1]);
    SetupPickupObject(&Objects[O_KEY_ITEM2]);
    SetupPickupObject(&Objects[O_KEY_ITEM3]);
    SetupPickupObject(&Objects[O_KEY_ITEM4]);
    SetupPickupObject(&Objects[O_PUZZLE_ITEM1]);
    SetupPickupObject(&Objects[O_PUZZLE_ITEM2]);
    SetupPickupObject(&Objects[O_PUZZLE_ITEM3]);
    SetupPickupObject(&Objects[O_PUZZLE_ITEM4]);
    SetupPickupObject(&Objects[O_GUN_ITEM]);
    SetupPickupObject(&Objects[O_SHOTGUN_ITEM]);
    SetupPickupObject(&Objects[O_MAGNUM_ITEM]);
    SetupPickupObject(&Objects[O_UZI_ITEM]);
    SetupPickupObject(&Objects[O_GUN_AMMO_ITEM]);
    SetupPickupObject(&Objects[O_SG_AMMO_ITEM]);
    SetupPickupObject(&Objects[O_MAG_AMMO_ITEM]);
    SetupPickupObject(&Objects[O_UZI_AMMO_ITEM]);
    SetupPickupObject(&Objects[O_EXPLOSIVE_ITEM]);
    SetupPickupObject(&Objects[O_MEDI_ITEM]);
    SetupPickupObject(&Objects[O_BIGMEDI_ITEM]);

    SetupScion1(&Objects[O_SCION_ITEM]);
    SetupScion2(&Objects[O_SCION_ITEM2]);
    SetupScion3(&Objects[O_SCION_ITEM3]);
    SetupScion4(&Objects[O_SCION_ITEM4]);
    SetupScionHolder(&Objects[O_SCION_HOLDER]);

    SetupLeadBar(&Objects[O_LEADBAR_ITEM]);
    SetupSaveGameCrystal(&Objects[O_SAVEGAME_ITEM]);
    SetupKeyHole(&Objects[O_KEY_HOLE1]);
    SetupKeyHole(&Objects[O_KEY_HOLE2]);
    SetupKeyHole(&Objects[O_KEY_HOLE3]);
    SetupKeyHole(&Objects[O_KEY_HOLE4]);

    SetupPuzzleHole(&Objects[O_PUZZLE_HOLE1]);
    SetupPuzzleHole(&Objects[O_PUZZLE_HOLE2]);
    SetupPuzzleHole(&Objects[O_PUZZLE_HOLE3]);
    SetupPuzzleHole(&Objects[O_PUZZLE_HOLE4]);
    SetupPuzzleDone(&Objects[O_PUZZLE_DONE1]);
    SetupPuzzleDone(&Objects[O_PUZZLE_DONE2]);
    SetupPuzzleDone(&Objects[O_PUZZLE_DONE3]);
    SetupPuzzleDone(&Objects[O_PUZZLE_DONE4]);

    SetupCabin(&Objects[O_PORTACABIN]);
    SetupBoat(&Objects[O_BOAT]);
    SetupEarthquake(&Objects[O_EARTHQUAKE]);

    Objects[O_PLAYER_1].initialise = InitialisePlayer1;
    Objects[O_PLAYER_1].control = ControlCinematicPlayer;
    Objects[O_PLAYER_1].hit_points = 1;

    Objects[O_PLAYER_2].initialise = InitialiseGenPlayer;
    Objects[O_PLAYER_2].control = ControlCinematicPlayer;
    Objects[O_PLAYER_2].hit_points = 1;

    Objects[O_PLAYER_3].initialise = InitialiseGenPlayer;
    Objects[O_PLAYER_3].control = ControlCinematicPlayer;
    Objects[O_PLAYER_3].hit_points = 1;

    Objects[O_PLAYER_4].initialise = InitialiseGenPlayer;
    Objects[O_PLAYER_4].control = ControlCinematicPlayer4;
    Objects[O_PLAYER_4].hit_points = 1;

    SetupBlood(&Objects[O_BLOOD1]);
    SetupBubble(&Objects[O_BUBBLES1]);
    SetupExplosion(&Objects[O_EXPLOSION1]);
    SetupRicochet(&Objects[O_RICOCHET1]);
    SetupTwinkle(&Objects[O_TWINKLE]);
    SetupSplash(&Objects[O_SPLASH1]);
    SetupWaterfall(&Objects[O_WATERFALL]);
    SetupBodyPart(&Objects[O_BODY_PART]);
    SetupNatlaGun(&Objects[O_MISSILE1]);
    SetupMissile(&Objects[O_MISSILE2]);
    SetupMissile(&Objects[O_MISSILE3]);
    SetupGunShot(&Objects[O_GUN_FLASH]);
}

void InitialiseObjects()
{
    for (int i = 0; i < O_NUMBER_OF; i++) {
        OBJECT_INFO *obj = &Objects[i];
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise = NULL;
        obj->collision = NULL;
        obj->control = NULL;
        obj->draw_routine = DrawAnimatingItem;
        obj->ceiling = NULL;
        obj->floor = NULL;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
    }

    BaddyObjects();
    TrapObjects();
    ObjectObjects();

    InitialiseHair();

    if (T1MConfig.disable_medpacks) {
        Objects[O_MEDI_ITEM].initialise = NULL;
        Objects[O_MEDI_ITEM].collision = NULL;
        Objects[O_MEDI_ITEM].control = NULL;
        Objects[O_MEDI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MEDI_ITEM].floor = NULL;
        Objects[O_MEDI_ITEM].ceiling = NULL;

        Objects[O_BIGMEDI_ITEM].initialise = NULL;
        Objects[O_BIGMEDI_ITEM].collision = NULL;
        Objects[O_BIGMEDI_ITEM].control = NULL;
        Objects[O_BIGMEDI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_BIGMEDI_ITEM].floor = NULL;
        Objects[O_BIGMEDI_ITEM].ceiling = NULL;
    }

    if (T1MConfig.disable_magnums) {
        Objects[O_MAGNUM_ITEM].initialise = NULL;
        Objects[O_MAGNUM_ITEM].collision = NULL;
        Objects[O_MAGNUM_ITEM].control = NULL;
        Objects[O_MAGNUM_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MAGNUM_ITEM].floor = NULL;
        Objects[O_MAGNUM_ITEM].ceiling = NULL;

        Objects[O_MAG_AMMO_ITEM].initialise = NULL;
        Objects[O_MAG_AMMO_ITEM].collision = NULL;
        Objects[O_MAG_AMMO_ITEM].control = NULL;
        Objects[O_MAG_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MAG_AMMO_ITEM].floor = NULL;
        Objects[O_MAG_AMMO_ITEM].ceiling = NULL;
    }

    if (T1MConfig.disable_uzis) {
        Objects[O_UZI_ITEM].initialise = NULL;
        Objects[O_UZI_ITEM].collision = NULL;
        Objects[O_UZI_ITEM].control = NULL;
        Objects[O_UZI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_UZI_ITEM].floor = NULL;
        Objects[O_UZI_ITEM].ceiling = NULL;

        Objects[O_UZI_AMMO_ITEM].initialise = NULL;
        Objects[O_UZI_AMMO_ITEM].collision = NULL;
        Objects[O_UZI_AMMO_ITEM].control = NULL;
        Objects[O_UZI_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_UZI_AMMO_ITEM].floor = NULL;
        Objects[O_UZI_AMMO_ITEM].ceiling = NULL;
    }

    if (T1MConfig.disable_shotgun) {
        Objects[O_SHOTGUN_ITEM].initialise = NULL;
        Objects[O_SHOTGUN_ITEM].collision = NULL;
        Objects[O_SHOTGUN_ITEM].control = NULL;
        Objects[O_SHOTGUN_ITEM].draw_routine = DrawDummyItem;
        Objects[O_SHOTGUN_ITEM].floor = NULL;
        Objects[O_SHOTGUN_ITEM].ceiling = NULL;

        Objects[O_SG_AMMO_ITEM].initialise = NULL;
        Objects[O_SG_AMMO_ITEM].collision = NULL;
        Objects[O_SG_AMMO_ITEM].control = NULL;
        Objects[O_SG_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_SG_AMMO_ITEM].floor = NULL;
        Objects[O_SG_AMMO_ITEM].ceiling = NULL;
    }
}

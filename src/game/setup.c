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
#include "game/gameflow.h"
#include "game/hair.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/level.h"
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

#include <stddef.h>

bool InitialiseLevel(int32_t level_num)
{
    LOG_DEBUG("%d", level_num);
    g_CurrentLevel = level_num;

    Text_RemoveAll();
    InitialiseGameFlags();

    InitialiseLaraLoad(NO_ITEM);
    if (level_num != g_GameFlow.title_level_num) {
        Screen_ApplyResolution();
    }

    if (!Level_Load(g_CurrentLevel)) {
        return false;
    }

    if (g_Lara.item_number != NO_ITEM) {
        InitialiseLara();
    }

    g_Effects = GameBuf_Alloc(NUM_EFFECTS * sizeof(FX_INFO), GBUF_EFFECTS);
    InitialiseFXArray();
    InitialiseLOTArray();

    Overlay_Init();
    Overlay_BarSetHealthTimer(100);

    Sound_ResetEffects();

    // LaraGun() expects request_gun_type to be set only when it really is
    // needed (see https://github.com/rr-/Tomb1Main/issues/36), not at all
    // times.
    g_Lara.request_gun_type = LGT_UNARMED;

    phd_AlterFOV(g_Config.fov_value * PHD_DEGREE);

    if (g_GameFlow.levels[g_CurrentLevel].music) {
        Music_PlayLooped(g_GameFlow.levels[g_CurrentLevel].music);
    }
    g_Camera.underwater = 0;
    return true;
}

void InitialiseGameFlags()
{
    g_FlipStatus = 0;
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        g_FlipMapTable[i] = 0;
    }

    for (int i = 0; i < MAX_CD_TRACKS; i++) {
        g_MusicTrackFlags[i] = 0;
    }

    /* Clear Object Loaded flags */
    for (int i = 0; i < O_NUMBER_OF; i++) {
        g_Objects[i].loaded = 0;
    }

    g_LevelComplete = false;
    g_FlipEffect = -1;
    g_PierreItemNum = NO_ITEM;
}

void InitialiseLevelFlags()
{
    g_GameInfo.stats.timer = 0;
    g_GameInfo.stats.secret_flags = 0;
    g_GameInfo.stats.pickup_count = 0;
    g_GameInfo.stats.kill_count = 0;
}

void BaddyObjects()
{
    g_Objects[O_LARA].initialise = InitialiseLaraLoad;
    g_Objects[O_LARA].draw_routine = DrawDummyItem;
    g_Objects[O_LARA].hit_points = g_Config.start_lara_hitpoints;
    g_Objects[O_LARA].shadow_size = (UNIT_SHADOW * 10) / 16;
    g_Objects[O_LARA].save_position = 1;
    g_Objects[O_LARA].save_hitpoints = 1;
    g_Objects[O_LARA].save_anim = 1;
    g_Objects[O_LARA].save_flags = 1;

    g_Objects[O_LARA_EXTRA].control = ControlLaraExtra;

    SetupEvilLara(&g_Objects[O_EVIL_LARA]);
    SetupWolf(&g_Objects[O_WOLF]);
    SetupBear(&g_Objects[O_BEAR]);
    SetupBat(&g_Objects[O_BAT]);
    SetupDino(&g_Objects[O_DINOSAUR]);
    SetupRaptor(&g_Objects[O_RAPTOR]);
    SetupLarson(&g_Objects[O_LARSON]);
    SetupPierre(&g_Objects[O_PIERRE]);
    SetupRat(&g_Objects[O_RAT]);
    SetupVole(&g_Objects[O_VOLE]);
    SetupLion(&g_Objects[O_LION]);
    SetupLioness(&g_Objects[O_LIONESS]);
    SetupPuma(&g_Objects[O_PUMA]);
    SetupCrocodile(&g_Objects[O_CROCODILE]);
    SetupAlligator(&g_Objects[O_ALLIGATOR]);
    SetupApe(&g_Objects[O_APE]);
    SetupWarrior(&g_Objects[O_WARRIOR1]);
    SetupWarrior2(&g_Objects[O_WARRIOR2]);
    SetupWarrior3(&g_Objects[O_WARRIOR3]);
    SetupCentaur(&g_Objects[O_CENTAUR]);
    SetupMummy(&g_Objects[O_MUMMY]);
    SetupSkateKid(&g_Objects[O_SKATEKID]);
    SetupCowboy(&g_Objects[O_COWBOY]);
    SetupBaldy(&g_Objects[O_BALDY]);
    SetupAbortion(&g_Objects[O_ABORTION]);
    SetupNatla(&g_Objects[O_NATLA]);
    SetupPod(&g_Objects[O_PODS]);
    SetupBigPod(&g_Objects[O_BIG_POD]);
    SetupStatue(&g_Objects[O_STATUE]);
}

void TrapObjects()
{
    SetupFallingBlock(&g_Objects[O_FALLING_BLOCK]);
    SetupPendulum(&g_Objects[O_PENDULUM]);
    SetupTeethTrap(&g_Objects[O_TEETH_TRAP]);
    SetupRollingBall(&g_Objects[O_ROLLING_BALL]);
    SetupSpikes(&g_Objects[O_SPIKES]);
    SetupFallingCeilling(&g_Objects[O_FALLING_CEILING1]);
    SetupFallingCeilling(&g_Objects[O_FALLING_CEILING2]);
    SetupDamoclesSword(&g_Objects[O_DAMOCLES_SWORD]);
    SetupMovableBlock(&g_Objects[O_MOVABLE_BLOCK]);
    SetupMovableBlock(&g_Objects[O_MOVABLE_BLOCK2]);
    SetupMovableBlock(&g_Objects[O_MOVABLE_BLOCK3]);
    SetupMovableBlock(&g_Objects[O_MOVABLE_BLOCK4]);
    SetupRollingBlock(&g_Objects[O_ROLLING_BLOCK]);
    SetupLightningEmitter(&g_Objects[O_LIGHTNING_EMITTER]);
    SetupThorsHandle(&g_Objects[O_THORS_HANDLE]);
    SetupThorsHead(&g_Objects[O_THORS_HEAD]);
    SetupMidasTouch(&g_Objects[O_MIDAS_TOUCH]);
    SetupDartEmitter(&g_Objects[O_DART_EMITTER]);
    SetupDart(&g_Objects[O_DARTS]);
    SetupDartEffect(&g_Objects[O_DART_EFFECT]);
    SetupFlameEmitter(&g_Objects[O_FLAME_EMITTER]);
    SetupFlame(&g_Objects[O_FLAME]);
    SetupLavaEmitter(&g_Objects[O_LAVA_EMITTER]);
    SetupLava(&g_Objects[O_LAVA]);
    SetupLavaWedge(&g_Objects[O_LAVA_WEDGE]);
}

void ObjectObjects()
{
    SetupCameraTarget(&g_Objects[O_CAMERA_TARGET]);
    SetupBridgeFlat(&g_Objects[O_BRIDGE_FLAT]);
    SetupBridgeTilt1(&g_Objects[O_BRIDGE_TILT1]);
    SetupBridgeTilt2(&g_Objects[O_BRIDGE_TILT2]);
    SetupDrawBridge(&g_Objects[O_DRAW_BRIDGE]);
    SetupSwitch1(&g_Objects[O_SWITCH_TYPE1]);
    SetupSwitch2(&g_Objects[O_SWITCH_TYPE2]);
    SetupDoor(&g_Objects[O_DOOR_TYPE1]);
    SetupDoor(&g_Objects[O_DOOR_TYPE2]);
    SetupDoor(&g_Objects[O_DOOR_TYPE3]);
    SetupDoor(&g_Objects[O_DOOR_TYPE4]);
    SetupDoor(&g_Objects[O_DOOR_TYPE5]);
    SetupDoor(&g_Objects[O_DOOR_TYPE6]);
    SetupDoor(&g_Objects[O_DOOR_TYPE7]);
    SetupDoor(&g_Objects[O_DOOR_TYPE8]);
    SetupTrapDoor(&g_Objects[O_TRAPDOOR]);
    SetupTrapDoor(&g_Objects[O_TRAPDOOR2]);
    SetupCog(&g_Objects[O_COG_1]);
    SetupCog(&g_Objects[O_COG_2]);
    SetupCog(&g_Objects[O_COG_3]);
    SetupMovingBar(&g_Objects[O_MOVING_BAR]);

    SetupPickupObject(&g_Objects[O_PICKUP_ITEM1]);
    SetupPickupObject(&g_Objects[O_PICKUP_ITEM2]);
    SetupPickupObject(&g_Objects[O_KEY_ITEM1]);
    SetupPickupObject(&g_Objects[O_KEY_ITEM2]);
    SetupPickupObject(&g_Objects[O_KEY_ITEM3]);
    SetupPickupObject(&g_Objects[O_KEY_ITEM4]);
    SetupPickupObject(&g_Objects[O_PUZZLE_ITEM1]);
    SetupPickupObject(&g_Objects[O_PUZZLE_ITEM2]);
    SetupPickupObject(&g_Objects[O_PUZZLE_ITEM3]);
    SetupPickupObject(&g_Objects[O_PUZZLE_ITEM4]);
    SetupPickupObject(&g_Objects[O_GUN_ITEM]);
    SetupPickupObject(&g_Objects[O_SHOTGUN_ITEM]);
    SetupPickupObject(&g_Objects[O_MAGNUM_ITEM]);
    SetupPickupObject(&g_Objects[O_UZI_ITEM]);
    SetupPickupObject(&g_Objects[O_GUN_AMMO_ITEM]);
    SetupPickupObject(&g_Objects[O_SG_AMMO_ITEM]);
    SetupPickupObject(&g_Objects[O_MAG_AMMO_ITEM]);
    SetupPickupObject(&g_Objects[O_UZI_AMMO_ITEM]);
    SetupPickupObject(&g_Objects[O_EXPLOSIVE_ITEM]);
    SetupPickupObject(&g_Objects[O_MEDI_ITEM]);
    SetupPickupObject(&g_Objects[O_BIGMEDI_ITEM]);

    SetupScion1(&g_Objects[O_SCION_ITEM]);
    SetupScion2(&g_Objects[O_SCION_ITEM2]);
    SetupScion3(&g_Objects[O_SCION_ITEM3]);
    SetupScion4(&g_Objects[O_SCION_ITEM4]);
    SetupScionHolder(&g_Objects[O_SCION_HOLDER]);

    SetupLeadBar(&g_Objects[O_LEADBAR_ITEM]);
    SetupSavegameCrystal(&g_Objects[O_SAVEGAME_ITEM]);
    SetupKeyHole(&g_Objects[O_KEY_HOLE1]);
    SetupKeyHole(&g_Objects[O_KEY_HOLE2]);
    SetupKeyHole(&g_Objects[O_KEY_HOLE3]);
    SetupKeyHole(&g_Objects[O_KEY_HOLE4]);

    SetupPuzzleHole(&g_Objects[O_PUZZLE_HOLE1]);
    SetupPuzzleHole(&g_Objects[O_PUZZLE_HOLE2]);
    SetupPuzzleHole(&g_Objects[O_PUZZLE_HOLE3]);
    SetupPuzzleHole(&g_Objects[O_PUZZLE_HOLE4]);
    SetupPuzzleDone(&g_Objects[O_PUZZLE_DONE1]);
    SetupPuzzleDone(&g_Objects[O_PUZZLE_DONE2]);
    SetupPuzzleDone(&g_Objects[O_PUZZLE_DONE3]);
    SetupPuzzleDone(&g_Objects[O_PUZZLE_DONE4]);

    SetupCabin(&g_Objects[O_PORTACABIN]);
    SetupBoat(&g_Objects[O_BOAT]);
    SetupEarthquake(&g_Objects[O_EARTHQUAKE]);

    g_Objects[O_PLAYER_1].initialise = InitialisePlayer1;
    g_Objects[O_PLAYER_1].control = ControlCinematicPlayer;
    g_Objects[O_PLAYER_1].hit_points = 1;

    g_Objects[O_PLAYER_2].initialise = InitialiseGenPlayer;
    g_Objects[O_PLAYER_2].control = ControlCinematicPlayer;
    g_Objects[O_PLAYER_2].hit_points = 1;

    g_Objects[O_PLAYER_3].initialise = InitialiseGenPlayer;
    g_Objects[O_PLAYER_3].control = ControlCinematicPlayer;
    g_Objects[O_PLAYER_3].hit_points = 1;

    g_Objects[O_PLAYER_4].initialise = InitialiseGenPlayer;
    g_Objects[O_PLAYER_4].control = ControlCinematicPlayer4;
    g_Objects[O_PLAYER_4].hit_points = 1;

    SetupBlood(&g_Objects[O_BLOOD1]);
    SetupBubble(&g_Objects[O_BUBBLES1]);
    SetupExplosion(&g_Objects[O_EXPLOSION1]);
    SetupRicochet(&g_Objects[O_RICOCHET1]);
    SetupTwinkle(&g_Objects[O_TWINKLE]);
    SetupSplash(&g_Objects[O_SPLASH1]);
    SetupWaterfall(&g_Objects[O_WATERFALL]);
    SetupBodyPart(&g_Objects[O_BODY_PART]);
    SetupNatlaGun(&g_Objects[O_MISSILE1]);
    SetupMissile(&g_Objects[O_MISSILE2]);
    SetupMissile(&g_Objects[O_MISSILE3]);
    SetupGunShot(&g_Objects[O_GUN_FLASH]);
}

void InitialiseObjects()
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

    if (g_Config.disable_medpacks) {
        g_Objects[O_MEDI_ITEM].initialise = NULL;
        g_Objects[O_MEDI_ITEM].collision = NULL;
        g_Objects[O_MEDI_ITEM].control = NULL;
        g_Objects[O_MEDI_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_MEDI_ITEM].floor = NULL;
        g_Objects[O_MEDI_ITEM].ceiling = NULL;

        g_Objects[O_BIGMEDI_ITEM].initialise = NULL;
        g_Objects[O_BIGMEDI_ITEM].collision = NULL;
        g_Objects[O_BIGMEDI_ITEM].control = NULL;
        g_Objects[O_BIGMEDI_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_BIGMEDI_ITEM].floor = NULL;
        g_Objects[O_BIGMEDI_ITEM].ceiling = NULL;
    }

    if (g_Config.disable_magnums) {
        g_Objects[O_MAGNUM_ITEM].initialise = NULL;
        g_Objects[O_MAGNUM_ITEM].collision = NULL;
        g_Objects[O_MAGNUM_ITEM].control = NULL;
        g_Objects[O_MAGNUM_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_MAGNUM_ITEM].floor = NULL;
        g_Objects[O_MAGNUM_ITEM].ceiling = NULL;

        g_Objects[O_MAG_AMMO_ITEM].initialise = NULL;
        g_Objects[O_MAG_AMMO_ITEM].collision = NULL;
        g_Objects[O_MAG_AMMO_ITEM].control = NULL;
        g_Objects[O_MAG_AMMO_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_MAG_AMMO_ITEM].floor = NULL;
        g_Objects[O_MAG_AMMO_ITEM].ceiling = NULL;
    }

    if (g_Config.disable_uzis) {
        g_Objects[O_UZI_ITEM].initialise = NULL;
        g_Objects[O_UZI_ITEM].collision = NULL;
        g_Objects[O_UZI_ITEM].control = NULL;
        g_Objects[O_UZI_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_UZI_ITEM].floor = NULL;
        g_Objects[O_UZI_ITEM].ceiling = NULL;

        g_Objects[O_UZI_AMMO_ITEM].initialise = NULL;
        g_Objects[O_UZI_AMMO_ITEM].collision = NULL;
        g_Objects[O_UZI_AMMO_ITEM].control = NULL;
        g_Objects[O_UZI_AMMO_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_UZI_AMMO_ITEM].floor = NULL;
        g_Objects[O_UZI_AMMO_ITEM].ceiling = NULL;
    }

    if (g_Config.disable_shotgun) {
        g_Objects[O_SHOTGUN_ITEM].initialise = NULL;
        g_Objects[O_SHOTGUN_ITEM].collision = NULL;
        g_Objects[O_SHOTGUN_ITEM].control = NULL;
        g_Objects[O_SHOTGUN_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_SHOTGUN_ITEM].floor = NULL;
        g_Objects[O_SHOTGUN_ITEM].ceiling = NULL;

        g_Objects[O_SG_AMMO_ITEM].initialise = NULL;
        g_Objects[O_SG_AMMO_ITEM].collision = NULL;
        g_Objects[O_SG_AMMO_ITEM].control = NULL;
        g_Objects[O_SG_AMMO_ITEM].draw_routine = DrawDummyItem;
        g_Objects[O_SG_AMMO_ITEM].floor = NULL;
        g_Objects[O_SG_AMMO_ITEM].ceiling = NULL;
    }
}

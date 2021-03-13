#include "3dsystem/3d_gen.h"
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
#include "game/box.h"
#include "game/cinema.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/hair.h"
#include "game/health.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/objects.h"
#include "game/objects/gunshot.h"
#include "game/objects/trapdoor.h"
#include "game/pickup.h"
#include "game/savegame.h"
#include "game/setup.h"
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
#include "game/types.h"
#include "game/vars.h"
#include "game/warrior.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/output.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "config.h"
#include "util.h"

int32_t InitialiseLevel(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type)
{
    // T1M: level_type argument is missing in OG
    TRACE("%d", level_num);
    if (level_type == GFL_SAVED) { // T1M: level_num == LV_CURRENT
        CurrentLevel = SaveGame.current_level;
    } else {
        CurrentLevel = level_num;
    }

    InitialiseGameFlags();

    Lara.item_number = NO_ITEM;

    S_InitialiseScreen();

    if (!S_LoadLevel(CurrentLevel)) {
        return 0;
    }

    if (Lara.item_number != NO_ITEM) {
        InitialiseLara();
    }

    Effects = game_malloc(NUM_EFFECTS * sizeof(FX_INFO), GBUF_EFFECTS);
    InitialiseFXArray();
    InitialiseLOTArray();

    InitColours();
    T_InitPrint();
    InitialisePickUpDisplay();

    HealthBarTimer = 100;
    mn_reset_sound_effects();

    if (level_type == GFL_SAVED) { // T1M: level_num == LV_CURRENT
        ExtractSaveGameInfo();
    }

    // LaraGun() expects request_gun_type to be set only when it really is
    // needed (see https://github.com/rr-/Tomb1Main/issues/36), not at all
    // times.
    Lara.request_gun_type = LGT_UNARMED;

    AlterFOV(T1MConfig.fov_value * PHD_DEGREE);

    if (GF.levels[CurrentLevel].music) {
        S_CDPlay(GF.levels[CurrentLevel].music);
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
        CDFlags[i] = 0;
    }

    /* Clear Object Loaded flags */
    for (int i = 0; i < NUMBER_OBJECTS; i++) {
        Objects[i].loaded = 0;
    }

    AmmoText = NULL;
    LevelComplete = 0;
    FlipEffect = -1;
    PierreItem = NO_ITEM;
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
    Objects[O_CAMERA_TARGET].draw_routine = DrawDummyItem;

    Objects[O_BRIDGE_FLAT].floor = BridgeFlatFloor;
    Objects[O_BRIDGE_FLAT].ceiling = BridgeFlatCeiling;
    Objects[O_BRIDGE_TILT1].floor = BridgeTilt1Floor;
    Objects[O_BRIDGE_TILT1].ceiling = BridgeTilt1Ceiling;
    Objects[O_BRIDGE_TILT2].floor = BridgeTilt2Floor;
    Objects[O_BRIDGE_TILT2].ceiling = BridgeTilt2Ceiling;

    if (Objects[O_DRAW_BRIDGE].loaded) {
        Objects[O_DRAW_BRIDGE].ceiling = DrawBridgeCeiling;
        Objects[O_DRAW_BRIDGE].collision = DrawBridgeCollision;
        Objects[O_DRAW_BRIDGE].control = CogControl;
        Objects[O_DRAW_BRIDGE].save_anim = 1;
        Objects[O_DRAW_BRIDGE].save_flags = 1;
        Objects[O_DRAW_BRIDGE].floor = DrawBridgeFloor;
    }

    Objects[O_SWITCH_TYPE1].control = SwitchControl;
    Objects[O_SWITCH_TYPE1].collision = SwitchCollision;
    Objects[O_SWITCH_TYPE1].save_anim = 1;
    Objects[O_SWITCH_TYPE1].save_flags = 1;

    Objects[O_SWITCH_TYPE2].control = SwitchControl;
    Objects[O_SWITCH_TYPE2].collision = SwitchCollision2;
    Objects[O_SWITCH_TYPE2].save_anim = 1;
    Objects[O_SWITCH_TYPE2].save_flags = 1;

    Objects[O_DOOR_TYPE1].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE1].control = DoorControl;
    Objects[O_DOOR_TYPE1].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE1].collision = DoorCollision;
    Objects[O_DOOR_TYPE1].save_anim = 1;
    Objects[O_DOOR_TYPE1].save_flags = 1;

    Objects[O_DOOR_TYPE2].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE2].control = DoorControl;
    Objects[O_DOOR_TYPE2].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE2].collision = DoorCollision;
    Objects[O_DOOR_TYPE2].save_anim = 1;
    Objects[O_DOOR_TYPE2].save_flags = 1;

    Objects[O_DOOR_TYPE3].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE3].control = DoorControl;
    Objects[O_DOOR_TYPE3].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE3].collision = DoorCollision;
    Objects[O_DOOR_TYPE3].save_anim = 1;
    Objects[O_DOOR_TYPE3].save_flags = 1;

    Objects[O_DOOR_TYPE4].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE4].control = DoorControl;
    Objects[O_DOOR_TYPE4].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE4].collision = DoorCollision;
    Objects[O_DOOR_TYPE4].save_anim = 1;
    Objects[O_DOOR_TYPE4].save_flags = 1;

    Objects[O_DOOR_TYPE5].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE5].control = DoorControl;
    Objects[O_DOOR_TYPE5].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE5].collision = DoorCollision;
    Objects[O_DOOR_TYPE5].save_anim = 1;
    Objects[O_DOOR_TYPE5].save_flags = 1;

    Objects[O_DOOR_TYPE6].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE6].control = DoorControl;
    Objects[O_DOOR_TYPE6].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE6].collision = DoorCollision;
    Objects[O_DOOR_TYPE6].save_anim = 1;
    Objects[O_DOOR_TYPE6].save_flags = 1;

    Objects[O_DOOR_TYPE7].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE7].control = DoorControl;
    Objects[O_DOOR_TYPE7].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE7].collision = DoorCollision;
    Objects[O_DOOR_TYPE7].save_anim = 1;
    Objects[O_DOOR_TYPE7].save_flags = 1;

    Objects[O_DOOR_TYPE8].initialise = InitialiseDoor;
    Objects[O_DOOR_TYPE8].control = DoorControl;
    Objects[O_DOOR_TYPE8].draw_routine = DrawUnclippedItem;
    Objects[O_DOOR_TYPE8].collision = DoorCollision;
    Objects[O_DOOR_TYPE8].save_anim = 1;
    Objects[O_DOOR_TYPE8].save_flags = 1;

    SetupTrapDoor(&Objects[O_TRAPDOOR]);
    SetupTrapDoor(&Objects[O_TRAPDOOR2]);

    Objects[O_COG_1].control = CogControl;
    Objects[O_COG_1].save_flags = 1;
    Objects[O_COG_2].control = CogControl;
    Objects[O_COG_2].save_flags = 1;
    Objects[O_COG_3].control = CogControl;
    Objects[O_COG_3].save_flags = 1;

    Objects[O_MOVING_BAR].control = CogControl;
    Objects[O_MOVING_BAR].collision = ObjectCollision;
    Objects[O_MOVING_BAR].save_flags = 1;
    Objects[O_MOVING_BAR].save_anim = 1;
    Objects[O_MOVING_BAR].save_position = 1;

    Objects[O_PICKUP_ITEM1].draw_routine = DrawSpriteItem;
    Objects[O_PICKUP_ITEM1].collision = PickUpCollision;
    Objects[O_PICKUP_ITEM1].save_flags = 1;

    Objects[O_PICKUP_ITEM2].draw_routine = DrawSpriteItem;
    Objects[O_PICKUP_ITEM2].collision = PickUpCollision;
    Objects[O_PICKUP_ITEM2].save_flags = 1;

    Objects[O_KEY_ITEM1].draw_routine = DrawSpriteItem;
    Objects[O_KEY_ITEM1].collision = PickUpCollision;
    Objects[O_KEY_ITEM1].save_flags = 1;

    Objects[O_KEY_ITEM2].draw_routine = DrawSpriteItem;
    Objects[O_KEY_ITEM2].collision = PickUpCollision;
    Objects[O_KEY_ITEM2].save_flags = 1;

    Objects[O_KEY_ITEM3].draw_routine = DrawSpriteItem;
    Objects[O_KEY_ITEM3].collision = PickUpCollision;
    Objects[O_KEY_ITEM3].save_flags = 1;

    Objects[O_KEY_ITEM4].draw_routine = DrawSpriteItem;
    Objects[O_KEY_ITEM4].collision = PickUpCollision;
    Objects[O_KEY_ITEM4].save_flags = 1;

    Objects[O_PUZZLE_ITEM1].draw_routine = DrawSpriteItem;
    Objects[O_PUZZLE_ITEM1].collision = PickUpCollision;
    Objects[O_PUZZLE_ITEM1].save_flags = 1;

    Objects[O_PUZZLE_ITEM2].draw_routine = DrawSpriteItem;
    Objects[O_PUZZLE_ITEM2].collision = PickUpCollision;
    Objects[O_PUZZLE_ITEM2].save_flags = 1;

    Objects[O_PUZZLE_ITEM3].draw_routine = DrawSpriteItem;
    Objects[O_PUZZLE_ITEM3].collision = PickUpCollision;
    Objects[O_PUZZLE_ITEM3].save_flags = 1;

    Objects[O_PUZZLE_ITEM4].draw_routine = DrawSpriteItem;
    Objects[O_PUZZLE_ITEM4].collision = PickUpCollision;
    Objects[O_PUZZLE_ITEM4].save_flags = 1;

    Objects[O_GUN_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_GUN_ITEM].collision = PickUpCollision;
    Objects[O_GUN_ITEM].save_flags = 1;

    Objects[O_SHOTGUN_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_SHOTGUN_ITEM].collision = PickUpCollision;
    Objects[O_SHOTGUN_ITEM].save_flags = 1;

    Objects[O_MAGNUM_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_MAGNUM_ITEM].collision = PickUpCollision;
    Objects[O_MAGNUM_ITEM].save_flags = 1;

    Objects[O_UZI_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_UZI_ITEM].collision = PickUpCollision;
    Objects[O_UZI_ITEM].save_flags = 1;

    Objects[O_GUN_AMMO_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_GUN_AMMO_ITEM].collision = PickUpCollision;
    Objects[O_GUN_AMMO_ITEM].save_flags = 1;

    Objects[O_SG_AMMO_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_SG_AMMO_ITEM].collision = PickUpCollision;
    Objects[O_SG_AMMO_ITEM].save_flags = 1;

    Objects[O_MAG_AMMO_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_MAG_AMMO_ITEM].collision = PickUpCollision;
    Objects[O_MAG_AMMO_ITEM].save_flags = 1;

    Objects[O_UZI_AMMO_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_UZI_AMMO_ITEM].collision = PickUpCollision;
    Objects[O_UZI_AMMO_ITEM].save_flags = 1;

    Objects[O_EXPLOSIVE_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_EXPLOSIVE_ITEM].collision = PickUpCollision;
    Objects[O_EXPLOSIVE_ITEM].save_flags = 1;

    Objects[O_MEDI_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_MEDI_ITEM].collision = PickUpCollision;
    Objects[O_MEDI_ITEM].save_flags = 1;

    Objects[O_BIGMEDI_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_BIGMEDI_ITEM].collision = PickUpCollision;
    Objects[O_BIGMEDI_ITEM].save_flags = 1;

    Objects[O_SCION_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_SCION_ITEM].collision = PickUpScionCollision;

    Objects[O_SCION_ITEM2].draw_routine = DrawSpriteItem;
    Objects[O_SCION_ITEM2].collision = PickUpCollision;
    Objects[O_SCION_ITEM2].save_flags = 1;

    Objects[O_SCION_ITEM3].control = Scion3Control;
    Objects[O_SCION_ITEM3].hit_points = 5;
    Objects[O_SCION_ITEM3].save_flags = 1;

    Objects[O_SCION_ITEM4].control = ScionControl;
    Objects[O_SCION_ITEM4].collision = PickUpScion4Collision;
    Objects[O_SCION_ITEM4].save_flags = 1;

    Objects[O_SCION_HOLDER].control = ScionControl;
    Objects[O_SCION_HOLDER].collision = ObjectCollision;
    Objects[O_SCION_HOLDER].save_anim = 1;
    Objects[O_SCION_HOLDER].save_flags = 1;

    Objects[O_LEADBAR_ITEM].draw_routine = DrawSpriteItem;
    Objects[O_LEADBAR_ITEM].collision = PickUpCollision;
    Objects[O_LEADBAR_ITEM].save_flags = 1;

    Objects[O_SAVEGAME_ITEM].initialise = InitialiseSaveGameItem;
#ifdef T1M_FEAT_SAVE_CRYSTALS
    Objects[O_SAVEGAME_ITEM].control = ControlSaveGameItem;
    Objects[O_SAVEGAME_ITEM].collision = PickUpSaveGameCollision;
    Objects[O_SAVEGAME_ITEM].save_flags = 1;
#endif

    Objects[O_KEY_HOLE1].collision = KeyHoleCollision;
    Objects[O_KEY_HOLE1].save_flags = 1;
    Objects[O_KEY_HOLE2].collision = KeyHoleCollision;
    Objects[O_KEY_HOLE2].save_flags = 1;
    Objects[O_KEY_HOLE3].collision = KeyHoleCollision;
    Objects[O_KEY_HOLE3].save_flags = 1;
    Objects[O_KEY_HOLE4].collision = KeyHoleCollision;
    Objects[O_KEY_HOLE4].save_flags = 1;

    Objects[O_PUZZLE_HOLE1].collision = PuzzleHoleCollision;
    Objects[O_PUZZLE_HOLE1].save_flags = 1;

    Objects[O_PUZZLE_HOLE2].collision = PuzzleHoleCollision;
    Objects[O_PUZZLE_HOLE2].save_flags = 1;

    Objects[O_PUZZLE_HOLE3].collision = PuzzleHoleCollision;
    Objects[O_PUZZLE_HOLE3].save_flags = 1;

    Objects[O_PUZZLE_HOLE4].collision = PuzzleHoleCollision;
    Objects[O_PUZZLE_HOLE4].save_flags = 1;

    Objects[O_PUZZLE_DONE1].save_flags = 1;
    Objects[O_PUZZLE_DONE2].save_flags = 1;
    Objects[O_PUZZLE_DONE3].save_flags = 1;
    Objects[O_PUZZLE_DONE4].save_flags = 1;

    Objects[O_PORTACABIN].control = CabinControl;
    Objects[O_PORTACABIN].draw_routine = DrawUnclippedItem;
    Objects[O_PORTACABIN].collision = ObjectCollision;
    Objects[O_PORTACABIN].save_anim = 1;
    Objects[O_PORTACABIN].save_flags = 1;

    Objects[O_BOAT].control = BoatControl;
    Objects[O_BOAT].save_flags = 1;
    Objects[O_BOAT].save_anim = 1;
    Objects[O_BOAT].save_position = 1;

    Objects[O_EARTHQUAKE].control = EarthQuakeControl;
    Objects[O_EARTHQUAKE].draw_routine = DrawDummyItem;
    Objects[O_EARTHQUAKE].save_flags = 1;

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

    Objects[O_BLOOD1].control = ControlBlood1;
    Objects[O_BUBBLES1].control = ControlBubble1;
    Objects[O_EXPLOSION1].control = ControlExplosion1;

    Objects[O_RICOCHET1].control = ControlRicochet1;
    Objects[O_TWINKLE].control = ControlTwinkle;
    Objects[O_SPLASH1].control = ControlSplash1;
    Objects[O_WATERFALL].control = ControlWaterFall;
    Objects[O_WATERFALL].draw_routine = DrawDummyItem;

    Objects[O_BODY_PART].control = ControlBodyPart;
    Objects[O_BODY_PART].nmeshes = 0;
    Objects[O_BODY_PART].loaded = 1;

    Objects[O_MISSILE1].control = ControlNatlaGun;
    Objects[O_MISSILE2].control = ControlMissile;
    Objects[O_MISSILE3].control = ControlMissile;

    SetupGunShot(&Objects[O_GUN_FLASH]);
}

void InitialiseObjects()
{
    for (int i = 0; i < NUMBER_OBJECTS; i++) {
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

void T1MInjectGameSetup()
{
    INJECT(0x004362A0, InitialiseLevel);
    INJECT(0x004363C0, InitialiseLevelFlags);

    INJECT(0x004363E0, BaddyObjects);
    INJECT(0x00437010, TrapObjects);
    INJECT(0x00437370, ObjectObjects);
    INJECT(0x00437A50, InitialiseObjects);
}

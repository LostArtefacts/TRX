#include "3dsystem/3d_gen.h"
#include "game/ai/alligator.h"
#include "game/ai/ape.h"
#include "game/ai/baldy.h"
#include "game/ai/bat.h"
#include "game/ai/bear.h"
#include "game/ai/cowboy.h"
#include "game/ai/croc.h"
#include "game/ai/dino.h"
#include "game/ai/evil_lara.h"
#include "game/ai/larson.h"
#include "game/ai/larson.h"
#include "game/ai/lion.h"
#include "game/ai/pierre.h"
#include "game/ai/raptor.h"
#include "game/ai/rat.h"
#include "game/ai/skate_kid.h"
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
#include "game/lightning.h"
#include "game/lot.h"
#include "game/moveblock.h"
#include "game/natla.h"
#include "game/objects.h"
#include "game/objects/gunshot.h"
#include "game/pickup.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/text.h"
#include "game/traps.h"
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
    SetupCroc(&Objects[O_CROCODILE]);
    SetupAlligator(&Objects[O_ALLIGATOR]);
    SetupApe(&Objects[O_APE]);

    if (Objects[O_WARRIOR1].loaded) {
        Objects[O_WARRIOR1].initialise = InitialiseCreature;
        Objects[O_WARRIOR1].control = FlyerControl;
        Objects[O_WARRIOR1].collision = CreatureCollision;
        Objects[O_WARRIOR1].shadow_size = UNIT_SHADOW / 3;
        Objects[O_WARRIOR1].hit_points = FLYER_HITPOINTS;
        Objects[O_WARRIOR1].pivot_length = 150;
        Objects[O_WARRIOR1].radius = FLYER_RADIUS;
        Objects[O_WARRIOR1].smartness = FLYER_SMARTNESS;
        Objects[O_WARRIOR1].intelligent = 1;
        Objects[O_WARRIOR1].save_position = 1;
        Objects[O_WARRIOR1].save_hitpoints = 1;
        Objects[O_WARRIOR1].save_anim = 1;
        Objects[O_WARRIOR1].save_flags = 1;
        AnimBones[Objects[O_WARRIOR1].bone_index] |= BEB_ROT_Y;
        AnimBones[Objects[O_WARRIOR1].bone_index + 8] |= BEB_ROT_Y;
    }

    if (Objects[O_WARRIOR2].loaded) {
        Objects[O_WARRIOR2] = Objects[O_WARRIOR1];
        Objects[O_WARRIOR2].initialise = InitialiseWarrior2;
        Objects[O_WARRIOR2].smartness = WARRIOR2_SMARTNESS;
    }

    if (Objects[O_WARRIOR3].loaded) {
        Objects[O_WARRIOR3] = Objects[O_WARRIOR1];
        Objects[O_WARRIOR3].initialise = InitialiseWarrior2;
        Objects[O_WARRIOR2].smartness = WARRIOR2_SMARTNESS; // sic
    }

    if (Objects[O_CENTAUR].loaded) {
        Objects[O_CENTAUR].initialise = InitialiseCreature;
        Objects[O_CENTAUR].control = CentaurControl;
        Objects[O_CENTAUR].collision = CreatureCollision;
        Objects[O_CENTAUR].shadow_size = UNIT_SHADOW / 3;
        Objects[O_CENTAUR].hit_points = CENTAUR_HITPOINTS;
        Objects[O_CENTAUR].pivot_length = 400;
        Objects[O_CENTAUR].radius = CENTAUR_RADIUS;
        Objects[O_CENTAUR].smartness = CENTAUR_SMARTNESS;
        Objects[O_CENTAUR].intelligent = 1;
        Objects[O_CENTAUR].save_position = 1;
        Objects[O_CENTAUR].save_hitpoints = 1;
        Objects[O_CENTAUR].save_anim = 1;
        Objects[O_CENTAUR].save_flags = 1;
        AnimBones[Objects[O_CENTAUR].bone_index + 40] |= 0xCu;
    }

    if (Objects[O_MUMMY].loaded) {
        Objects[O_MUMMY].initialise = InitialiseMummy;
        Objects[O_MUMMY].control = MummyControl;
        Objects[O_MUMMY].collision = ObjectCollision;
        Objects[O_MUMMY].hit_points = MUMMY_HITPOINTS;
        Objects[O_MUMMY].save_flags = 1;
        Objects[O_MUMMY].save_hitpoints = 1;
        Objects[O_MUMMY].save_anim = 1;
        AnimBones[Objects[O_MUMMY].bone_index + 8] |= BEB_ROT_Y;
    }

    SetupSkateKid(&Objects[O_MERCENARY1]);
    SetupCowboy(&Objects[O_MERCENARY2]);
    SetupBaldy(&Objects[O_MERCENARY3]);

    if (Objects[O_ABORTION].loaded) {
        Objects[O_ABORTION].initialise = InitialiseCreature;
        Objects[O_ABORTION].control = AbortionControl;
        Objects[O_ABORTION].collision = CreatureCollision;
        Objects[O_ABORTION].shadow_size = UNIT_SHADOW / 3;
        Objects[O_ABORTION].hit_points = ABORTION_HITPOINTS;
        Objects[O_ABORTION].radius = ABORTION_RADIUS;
        Objects[O_ABORTION].smartness = ABORTION_SMARTNESS;
        Objects[O_ABORTION].intelligent = 1;
        Objects[O_ABORTION].save_position = 1;
        Objects[O_ABORTION].save_hitpoints = 1;
        Objects[O_ABORTION].save_anim = 1;
        Objects[O_ABORTION].save_flags = 1;
        AnimBones[Objects[O_ABORTION].bone_index + 4] |= BEB_ROT_Y;
    }

    if (Objects[O_NATLA].loaded) {
        Objects[O_NATLA].collision = CreatureCollision;
        Objects[O_NATLA].initialise = InitialiseCreature;
        Objects[O_NATLA].control = NatlaControl;
        Objects[O_NATLA].shadow_size = UNIT_SHADOW / 2;
        Objects[O_NATLA].hit_points = NATLA_HITPOINTS;
        Objects[O_NATLA].radius = NATLA_RADIUS;
        Objects[O_NATLA].smartness = NATLA_SMARTNESS;
        Objects[O_NATLA].intelligent = 1;
        Objects[O_NATLA].save_position = 1;
        Objects[O_NATLA].save_hitpoints = 1;
        Objects[O_NATLA].save_anim = 1;
        Objects[O_NATLA].save_flags = 1;
        AnimBones[Objects[O_NATLA].bone_index + 8] |= BEB_ROT_Z | BEB_ROT_X;
    }

    if (Objects[O_PODS].loaded) {
        Objects[O_PODS].initialise = InitialisePod;
        Objects[O_PODS].control = PodControl;
        Objects[O_PODS].collision = ObjectCollision;
        Objects[O_PODS].save_anim = 1;
        Objects[O_PODS].save_flags = 1;
    }

    if (Objects[O_BIG_POD].loaded) {
        Objects[O_BIG_POD].initialise = InitialisePod;
        Objects[O_BIG_POD].control = PodControl;
        Objects[O_BIG_POD].collision = ObjectCollision;
        Objects[O_BIG_POD].save_anim = 1;
        Objects[O_BIG_POD].save_flags = 1;
    }

    if (Objects[O_STATUE].loaded) {
        Objects[O_STATUE].initialise = InitialiseStatue;
        Objects[O_STATUE].control = StatueControl;
        Objects[O_STATUE].collision = ObjectCollision;
        Objects[O_STATUE].save_anim = 1;
        Objects[O_STATUE].save_flags = 1;
    }
}

void TrapObjects()
{
    Objects[O_FALLING_BLOCK].control = FallingBlockControl;
    Objects[O_FALLING_BLOCK].floor = FallingBlockFloor;
    Objects[O_FALLING_BLOCK].ceiling = FallingBlockCeiling;
    Objects[O_FALLING_BLOCK].save_position = 1;
    Objects[O_FALLING_BLOCK].save_anim = 1;
    Objects[O_FALLING_BLOCK].save_flags = 1;

    Objects[O_PENDULUM].control = PendulumControl;
    Objects[O_PENDULUM].collision = TrapCollision;
    Objects[O_PENDULUM].shadow_size = UNIT_SHADOW / 2;
    Objects[O_PENDULUM].save_flags = 1;
    Objects[O_PENDULUM].save_anim = 1;

    Objects[O_TEETH_TRAP].control = TeethTrapControl;
    Objects[O_TEETH_TRAP].collision = TrapCollision;
    Objects[O_TEETH_TRAP].save_flags = 1;
    Objects[O_TEETH_TRAP].save_anim = 1;

    Objects[O_ROLLING_BALL].initialise = InitialiseRollingBall;
    Objects[O_ROLLING_BALL].control = RollingBallControl;
    Objects[O_ROLLING_BALL].collision = RollingBallCollision;
    Objects[O_ROLLING_BALL].save_position = 1;
    Objects[O_ROLLING_BALL].save_anim = 1;
    Objects[O_ROLLING_BALL].save_flags = 1;

    Objects[O_SPIKES].collision = SpikeCollision;

    Objects[O_FALLING_CEILING1].control = FallingCeilingControl;
    Objects[O_FALLING_CEILING1].collision = TrapCollision;
    Objects[O_FALLING_CEILING1].save_position = 1;
    Objects[O_FALLING_CEILING1].save_anim = 1;
    Objects[O_FALLING_CEILING1].save_flags = 1;

    Objects[O_FALLING_CEILING2].control = FallingCeilingControl;
    Objects[O_FALLING_CEILING2].collision = TrapCollision;
    Objects[O_FALLING_CEILING2].save_position = 1;
    Objects[O_FALLING_CEILING2].save_anim = 1;
    Objects[O_FALLING_CEILING2].save_flags = 1;

    Objects[O_DAMOCLES_SWORD].initialise = InitialiseDamoclesSword;
    Objects[O_DAMOCLES_SWORD].control = DamoclesSwordControl;
    Objects[O_DAMOCLES_SWORD].collision = DamoclesSwordCollision;
    Objects[O_DAMOCLES_SWORD].shadow_size = UNIT_SHADOW;
    Objects[O_DAMOCLES_SWORD].save_position = 1;
    Objects[O_DAMOCLES_SWORD].save_anim = 1;
    Objects[O_DAMOCLES_SWORD].save_flags = 1;

    Objects[O_MOVABLE_BLOCK].initialise = InitialiseMovableBlock;
    Objects[O_MOVABLE_BLOCK].control = MovableBlockControl;
    Objects[O_MOVABLE_BLOCK].draw_routine = DrawMovableBlock;
    Objects[O_MOVABLE_BLOCK].collision = MovableBlockCollision;
    Objects[O_MOVABLE_BLOCK].save_position = 1;
    Objects[O_MOVABLE_BLOCK].save_anim = 1;
    Objects[O_MOVABLE_BLOCK].save_flags = 1;

    Objects[O_MOVABLE_BLOCK2].initialise = InitialiseMovableBlock;
    Objects[O_MOVABLE_BLOCK2].control = MovableBlockControl;
    Objects[O_MOVABLE_BLOCK2].draw_routine = DrawMovableBlock;
    Objects[O_MOVABLE_BLOCK2].collision = MovableBlockCollision;
    Objects[O_MOVABLE_BLOCK2].save_position = 1;
    Objects[O_MOVABLE_BLOCK2].save_anim = 1;
    Objects[O_MOVABLE_BLOCK2].save_flags = 1;

    Objects[O_MOVABLE_BLOCK3].initialise = InitialiseMovableBlock;
    Objects[O_MOVABLE_BLOCK3].draw_routine = DrawMovableBlock;
    Objects[O_MOVABLE_BLOCK3].control = MovableBlockControl;
    Objects[O_MOVABLE_BLOCK3].collision = MovableBlockCollision;
    Objects[O_MOVABLE_BLOCK3].save_position = 1;
    Objects[O_MOVABLE_BLOCK3].save_anim = 1;
    Objects[O_MOVABLE_BLOCK3].save_flags = 1;

    Objects[O_MOVABLE_BLOCK4].initialise = InitialiseMovableBlock;
    Objects[O_MOVABLE_BLOCK4].control = MovableBlockControl;
    Objects[O_MOVABLE_BLOCK4].draw_routine = DrawMovableBlock;
    Objects[O_MOVABLE_BLOCK4].collision = MovableBlockCollision;
    Objects[O_MOVABLE_BLOCK4].save_position = 1;
    Objects[O_MOVABLE_BLOCK4].save_anim = 1;
    Objects[O_MOVABLE_BLOCK4].save_flags = 1;

    Objects[O_ROLLING_BLOCK].initialise = InitialiseRollingBlock;
    Objects[O_ROLLING_BLOCK].control = RollingBlockControl;
    Objects[O_ROLLING_BLOCK].save_position = 1;
    Objects[O_ROLLING_BLOCK].save_anim = 1;
    Objects[O_ROLLING_BLOCK].save_flags = 1;

    Objects[O_LIGHTNING_EMITTER].initialise = InitialiseLightning;
    Objects[O_LIGHTNING_EMITTER].control = LightningControl;
    Objects[O_LIGHTNING_EMITTER].draw_routine = DrawLightning;
    Objects[O_LIGHTNING_EMITTER].collision = LightningCollision;
    Objects[O_LIGHTNING_EMITTER].save_flags = 1;

    Objects[O_THORS_HANDLE].initialise = InitialiseThorsHandle;
    Objects[O_THORS_HANDLE].control = ThorsHandleControl;
    Objects[O_THORS_HANDLE].draw_routine = DrawUnclippedItem;
    Objects[O_THORS_HANDLE].collision = ThorsHandleCollision;
    Objects[O_THORS_HANDLE].save_flags = 1;
    Objects[O_THORS_HANDLE].save_anim = 1;

    Objects[O_THORS_HEAD].collision = ThorsHeadCollision;
    Objects[O_THORS_HEAD].draw_routine = DrawUnclippedItem;
    Objects[O_THORS_HEAD].save_flags = 1;
    Objects[O_THORS_HEAD].save_anim = 1;

    Objects[O_MIDAS_TOUCH].collision = MidasCollision;
    Objects[O_MIDAS_TOUCH].draw_routine = DrawDummyItem;

    Objects[O_DART_EMITTER].control = DartEmitterControl;

    Objects[O_DARTS].collision = ObjectCollision;
    Objects[O_DARTS].control = DartsControl;
    Objects[O_DARTS].shadow_size = UNIT_SHADOW / 2;
    Objects[O_DARTS].save_flags = 1;

    Objects[O_DART_EFFECT].control = DartEffectControl;
    Objects[O_DART_EFFECT].draw_routine = DrawSpriteItem;

    Objects[O_FLAME_EMITTER].control = FlameEmitterControl;
    Objects[O_FLAME_EMITTER].draw_routine = DrawDummyItem;

    Objects[O_FLAME].control = FlameControl;

    Objects[O_LAVA_EMITTER].control = LavaEmitterControl;
    Objects[O_LAVA_EMITTER].draw_routine = DrawDummyItem;
    Objects[O_LAVA_EMITTER].collision = ObjectCollision;

    Objects[O_LAVA].control = LavaControl;

    Objects[O_LAVA_WEDGE].control = LavaWedgeControl;
    Objects[O_LAVA_WEDGE].collision = CreatureCollision;
    Objects[O_LAVA_WEDGE].save_position = 1;
    Objects[O_LAVA_WEDGE].save_anim = 1;
    Objects[O_LAVA_WEDGE].save_flags = 1;
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

    Objects[O_TRAPDOOR].control = TrapDoorControl;
    Objects[O_TRAPDOOR].floor = TrapDoorFloor;
    Objects[O_TRAPDOOR].ceiling = TrapDoorCeiling;
    Objects[O_TRAPDOOR].save_anim = 1;
    Objects[O_TRAPDOOR].save_flags = 1;

    Objects[O_TRAPDOOR2].control = TrapDoorControl;
    Objects[O_TRAPDOOR2].floor = TrapDoorFloor;
    Objects[O_TRAPDOOR2].ceiling = TrapDoorCeiling;
    Objects[O_TRAPDOOR2].save_anim = 1;
    Objects[O_TRAPDOOR2].save_flags = 1;

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

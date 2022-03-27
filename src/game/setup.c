#include "game/setup.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "game/cinema.h"
#include "game/draw.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/hair.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/ai/alligator.h"
#include "game/objects/ai/ape.h"
#include "game/objects/ai/baldy.h"
#include "game/objects/ai/bat.h"
#include "game/objects/ai/bear.h"
#include "game/objects/ai/centaur.h"
#include "game/objects/ai/cowboy.h"
#include "game/objects/ai/crocodile.h"
#include "game/objects/ai/evil_lara.h"
#include "game/objects/ai/larson.h"
#include "game/objects/ai/lion.h"
#include "game/objects/ai/mummy.h"
#include "game/objects/ai/mutant.h"
#include "game/objects/ai/natla.h"
#include "game/objects/ai/pierre.h"
#include "game/objects/ai/pod.h"
#include "game/objects/ai/raptor.h"
#include "game/objects/ai/rat.h"
#include "game/objects/ai/skate_kid.h"
#include "game/objects/ai/statue.h"
#include "game/objects/ai/torso.h"
#include "game/objects/ai/trex.h"
#include "game/objects/ai/vole.h"
#include "game/objects/ai/wolf.h"
#include "game/objects/boat.h"
#include "game/objects/bridge.h"
#include "game/objects/cabin.h"
#include "game/objects/cog.h"
#include "game/objects/door.h"
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
#include "game/objects/keyhole.h"
#include "game/objects/misc.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/objects/save_crystal.h"
#include "game/objects/scion.h"
#include "game/objects/switch.h"
#include "game/objects/trapdoor.h"
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
#include "game/objects/traps/rolling_block.h"
#include "game/objects/traps/spikes.h"
#include "game/objects/traps/teeth_trap.h"
#include "game/objects/traps/thors_hammer.h"
#include "game/overlay.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/sound.h"
#include "game/text.h"
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

    phd_AlterFOV(g_Config.fov_value * PHD_DEGREE);

    if (g_GameFlow.levels[g_CurrentLevel].music) {
        Music_PlayLooped(g_GameFlow.levels[g_CurrentLevel].music);
    }
    g_Camera.underwater = 0;
    return true;
}

void InitialiseGameFlags(void)
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

void InitialiseLevelFlags(void)
{
    // loading a save can override it to false
    g_GameInfo.death_counter_supported = true;

    g_GameInfo.stats.timer = 0;
    g_GameInfo.stats.secret_flags = 0;
    g_GameInfo.stats.pickup_count = 0;
    g_GameInfo.stats.kill_count = 0;
    g_GameInfo.stats.death_count = 0;
}

void BaddyObjects(void)
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
    Bear_Setup(&g_Objects[O_BEAR]);
    Bat_Setup(&g_Objects[O_BAT]);
    TRex_Setup(&g_Objects[O_TREX]);
    SetupRaptor(&g_Objects[O_RAPTOR]);
    SetupLarson(&g_Objects[O_LARSON]);
    SetupPierre(&g_Objects[O_PIERRE]);
    SetupRat(&g_Objects[O_RAT]);
    SetupVole(&g_Objects[O_VOLE]);
    SetupLion(&g_Objects[O_LION]);
    SetupLioness(&g_Objects[O_LIONESS]);
    SetupPuma(&g_Objects[O_PUMA]);
    Croc_Setup(&g_Objects[O_CROCODILE]);
    Alligator_Setup(&g_Objects[O_ALLIGATOR]);
    Ape_Setup(&g_Objects[O_APE]);
    SetupWarrior(&g_Objects[O_WARRIOR1]);
    SetupWarrior2(&g_Objects[O_WARRIOR2]);
    SetupWarrior3(&g_Objects[O_WARRIOR3]);
    Centaur_Setup(&g_Objects[O_CENTAUR]);
    SetupMummy(&g_Objects[O_MUMMY]);
    SetupSkateKid(&g_Objects[O_SKATEKID]);
    Cowboy_Setup(&g_Objects[O_COWBOY]);
    Baldy_Setup(&g_Objects[O_BALDY]);
    Torso_Setup(&g_Objects[O_TORSO]);
    SetupNatla(&g_Objects[O_NATLA]);
    SetupPod(&g_Objects[O_PODS]);
    SetupBigPod(&g_Objects[O_BIG_POD]);
    SetupStatue(&g_Objects[O_STATUE]);
}

void TrapObjects(void)
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
    RollingBlock_Setup(&g_Objects[O_ROLLING_BLOCK]);
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

void ObjectObjects(void)
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
    Pickup_Setup(&g_Objects[O_GUN_ITEM]);
    Pickup_Setup(&g_Objects[O_SHOTGUN_ITEM]);
    Pickup_Setup(&g_Objects[O_MAGNUM_ITEM]);
    Pickup_Setup(&g_Objects[O_UZI_ITEM]);
    Pickup_Setup(&g_Objects[O_GUN_AMMO_ITEM]);
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

    Blood_Setup(&g_Objects[O_BLOOD1]);
    Bubble_Setup(&g_Objects[O_BUBBLES1]);
    Explosion_Setup(&g_Objects[O_EXPLOSION1]);
    Ricochet_Setup(&g_Objects[O_RICOCHET1]);
    Twinkle_Setup(&g_Objects[O_TWINKLE]);
    Splash_Setup(&g_Objects[O_SPLASH1]);
    Waterfall_Setup(&g_Objects[O_WATERFALL]);
    BodyPart_Setup(&g_Objects[O_BODY_PART]);
    SetupNatlaGun(&g_Objects[O_MISSILE1]);
    Missile_Setup(&g_Objects[O_MISSILE2]);
    Missile_Setup(&g_Objects[O_MISSILE3]);
    GunShot_Setup(&g_Objects[O_GUN_FLASH]);
}

void InitialiseObjects(void)
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

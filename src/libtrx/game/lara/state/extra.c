#include "game/camera.h"
#include "game/game.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/lara/util.h"
#include "game/music.h"
#include "game/objects/effects/twinkle.h"
#include "game/overlay.h"
#include "game/rooms.h"
#include "game/viewport.h"

// clang-format off
#define M_LF_PICKUP_GOLD_BAR           113
#define M_LF_SHARK_DEATH_END           56
#define M_LF_SHARK_DEATH_TIMER_DELAY   25
#define M_LF_TREX_DEATH_TIMER_DELAY    45
#define M_LF_YETI_DEATH_TIMER_DELAY    70
#define M_LF_DRAGON_DAGGER_PULLED      1
#define M_LF_DRAGON_DAGGER_STORED      180
#define M_LF_DRAGON_DAGGER_DISPLAY     210
#define M_LF_DRAGON_DAGGER_ANIM_END    239
#define M_LF_START_HOUSE_BEGIN         1
#define M_LF_START_HOUSE_DAGGER_STORED 401
#define M_LF_START_HOUSE_END           427
#define M_LF_SHOWER_START              1
#define M_LF_SHOWER_SHOTGUN_PICKUP     316
#define M_CAM_YETI_KILL_ANGLE         (160 * DEG_1) // = 29120
#define M_CAM_YETI_KILL_DISTANCE      (3 * WALL_L)  // = 3072
#define M_CAM_SHARK_KILL_ANGLE        (160 * DEG_1) // = 29120
#define M_CAM_SHARK_KILL_DISTANCE     (3 * WALL_L)  // = 3072
#define M_CAM_AIRLOCK_ANGLE           (80 * DEG_1)  // = 14560
#define M_CAM_AIRLOCK_ELEVATION       (-25 * DEG_1) // = -4550
#define M_CAM_GONG_BONG_ANGLE         (-25 * DEG_1) // = -4550
#define M_CAM_GONG_BONG_ELEVATION     (-20 * DEG_1) // = -3640
#define M_CAM_GONG_BONG_DISTANCE      (3 * WALL_L)  // = 3072
#define M_CAM_TREX_KILL_ANGLE         (170 * DEG_1) // = 30940
#define M_CAM_TREX_KILL_ELEVATION     (-25 * DEG_1) // = -4550
// clang-format on

static void M_UseMidas(ITEM *const item, COLL_INFO *const coll)
{
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;
    Twinkle_SparkleItem(item, (1 << LM_HAND_L) | (1 << LM_HAND_R));

    if (Item_TestFrameEqual(item, M_LF_PICKUP_GOLD_BAR)) {
        Overlay_AddDisplayPickup(O_PUZZLE_ITEM_1);
        Inv_RemoveItem(O_LEADBAR_ITEM);
        Inv_AddItem(O_PUZZLE_ITEM_1);
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->interact_target.item_num = NO_ITEM;
    }
}

static void M_DieMidas(ITEM *const item, COLL_INFO *const coll)
{
    item->gravity = false;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    Object_SetReflective(O_LARA_EXTRA, true);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    const int32_t frame_num = Item_GetRelativeFrame(item);
    switch (frame_num) {
    case 5:
        lara->mesh_effects |= (1 << LM_FOOT_L);
        lara->mesh_effects |= (1 << LM_FOOT_R);
        Lara_Mesh_SwapSingle(LM_FOOT_L, O_LARA_EXTRA);
        Lara_Mesh_SwapSingle(LM_FOOT_R, O_LARA_EXTRA);
        break;

    case 70:
        lara->mesh_effects |= (1 << LM_CALF_L);
        Lara_Mesh_SwapSingle(LM_CALF_L, O_LARA_EXTRA);
        break;

    case 90:
        lara->mesh_effects |= (1 << LM_THIGH_L);
        Lara_Mesh_SwapSingle(LM_THIGH_L, O_LARA_EXTRA);
        break;

    case 100:
        lara->mesh_effects |= (1 << LM_CALF_R);
        Lara_Mesh_SwapSingle(LM_CALF_R, O_LARA_EXTRA);
        break;

    case 120:
        lara->mesh_effects |= (1 << LM_HIPS);
        lara->mesh_effects |= (1 << LM_THIGH_R);
        Lara_Mesh_SwapSingle(LM_HIPS, O_LARA_EXTRA);
        Lara_Mesh_SwapSingle(LM_THIGH_R, O_LARA_EXTRA);
        break;

    case 135:
        lara->mesh_effects |= (1 << LM_TORSO);
        Lara_Mesh_SwapSingle(LM_TORSO, O_LARA_EXTRA);
        break;

    case 150:
        lara->mesh_effects |= (1 << LM_UARM_L);
        Lara_Mesh_SwapSingle(LM_UARM_L, O_LARA_EXTRA);
        break;

    case 163:
        lara->mesh_effects |= (1 << LM_LARM_L);
        Lara_Mesh_SwapSingle(LM_LARM_L, O_LARA_EXTRA);
        break;

    case 174:
        lara->mesh_effects |= (1 << LM_HAND_L);
        Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA_EXTRA);
        break;

    case 186:
        lara->mesh_effects |= (1 << LM_UARM_R);
        Lara_Mesh_SwapSingle(LM_UARM_R, O_LARA_EXTRA);
        break;

    case 195:
        lara->mesh_effects |= (1 << LM_LARM_R);
        Lara_Mesh_SwapSingle(LM_LARM_R, O_LARA_EXTRA);
        break;

    case 218:
        lara->mesh_effects |= (1 << LM_HAND_R);
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA_EXTRA);
        break;

    case 225:
        Object_SetReflective(O_LARA_HAIR, true);
        lara->mesh_effects |= (1 << LM_HEAD);
        Lara_Mesh_SwapSingle(LM_HEAD, O_LARA_EXTRA);
        break;
    }

    Twinkle_SparkleItem(item, lara->mesh_effects);
}

static void M_Breath(ITEM *const item, COLL_INFO *const coll)
{
    Item_SwitchToAnim(item, LA(LA_STAND_IDLE), 0);
    item->goal_anim_state = LS(LS_STOP);
    item->current_anim_state = LS(LS_STOP);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->extra_anim = false;
    lara->gun_status = LGS_ARMLESS;
    if (g_Camera.type != CAM_HEAVY) {
        g_Camera.type = CAM_CHASE;
    }
#if TR_VERSION == 2
    Viewport_AlterFOV(-1);
#endif
}

static void M_YetiKill(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = M_CAM_YETI_KILL_ANGLE;
    g_Camera.target_distance = M_CAM_YETI_KILL_DISTANCE;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = -1;
    if (Item_TestFrameRange(item, 0, M_LF_YETI_DEATH_TIMER_DELAY)) {
        lara->death_timer = 1;
    }
}

static void M_SharkKill(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = M_CAM_SHARK_KILL_ANGLE;
    g_Camera.target_distance = M_CAM_SHARK_KILL_DISTANCE;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = -1;

    if (Item_TestFrameEqual(item, M_LF_SHARK_DEATH_END)) {
        const int32_t water_height = Room_GetWaterHeight(
            item->pos.x, item->pos.y, item->pos.z, item->room_num);
        if (water_height != NO_HEIGHT && water_height < item->pos.y - 100) {
            item->pos.y -= 5;
        }
    }

    if (Item_TestFrameRange(item, 0, M_LF_SHARK_DEATH_TIMER_DELAY)) {
        lara->death_timer = 1;
    }
}

static void M_Airlock(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = M_CAM_AIRLOCK_ANGLE;
    g_Camera.target_elevation = M_CAM_AIRLOCK_ELEVATION;
}

static void M_GongBong(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.target_angle = M_CAM_GONG_BONG_ANGLE;
    g_Camera.target_elevation = M_CAM_GONG_BONG_ELEVATION;
    g_Camera.target_distance = M_CAM_GONG_BONG_DISTANCE;
}

static void M_DinoKill(ITEM *const item, COLL_INFO *const coll)
{
    g_Camera.flags = CF_FOLLOW_CENTRE;
    g_Camera.target_angle = M_CAM_TREX_KILL_ANGLE;
    g_Camera.target_elevation = M_CAM_TREX_KILL_ELEVATION;
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = -1;
    if (Item_TestFrameRange(item, 0, M_LF_TREX_DEATH_TIMER_DELAY)) {
        lara->death_timer = 1;
    }
}

static void M_PullDagger(ITEM *const item, COLL_INFO *const coll)
{
    if (Item_TestFrameEqual(item, M_LF_DRAGON_DAGGER_PULLED)) {
        Music_Play(MX_DAGGER_PULL, MPM_ALWAYS);
    } else if (Item_TestFrameEqual(item, M_LF_DRAGON_DAGGER_STORED)) {
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA);
        Inv_AddItem(O_PUZZLE_ITEM_2);
    } else if (Item_TestFrameEqual(item, M_LF_DRAGON_DAGGER_DISPLAY)) {
        Overlay_AddDisplayPickup(O_PUZZLE_ITEM_2);
    } else if (Item_TestFrameEqual(item, M_LF_DRAGON_DAGGER_ANIM_END)) {
        item->rot.y += DEG_90;

        const ITEM *const dragon_bones = Item_Find(O_DRAGON_BONES_2);
        if (dragon_bones != nullptr) {
            Room_TestTriggers(dragon_bones);
        }
    }
}

static void M_StartAnim(ITEM *const item, COLL_INFO *const coll)
{
    Room_TestTriggers(item);
}

static void M_StartHouse(ITEM *const item, COLL_INFO *const coll)
{
    if (Item_TestFrameEqual(item, M_LF_START_HOUSE_BEGIN)) {
        Music_Play(MX_REVEAL_2, MPM_ALWAYS);
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA_EXTRA);
        Lara_Mesh_SwapSingle(LM_HIPS, O_LARA_EXTRA);
    } else if (Item_TestFrameEqual(item, M_LF_START_HOUSE_DAGGER_STORED)) {
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA);
        Lara_Mesh_SwapSingle(LM_HIPS, O_LARA);
        Inv_AddItem(O_PUZZLE_ITEM_1);
    } else if (Item_TestFrameEqual(item, M_LF_START_HOUSE_END)) {
        g_Camera.type = CAM_CHASE;
        Viewport_AlterFOV(-1);
    }
}

static void M_FinalAnim(ITEM *const item, COLL_INFO *const coll)
{
    item->hit_points = LARA_MAX_HITPOINTS;
    Lara_SetControllable(false);

    if (Item_TestFrameEqual(item, M_LF_SHOWER_START)) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
#if TR_VERSION == 2
        lara->back_gun_obj_id = O_LARA;
#endif
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA);
        Lara_Mesh_SwapSingle(LM_HEAD, O_LARA);
        Lara_Mesh_SwapSingle(LM_HIPS, O_LARA_EXTRA);
        Music_Play(MX_CUTSCENE_BATH, MPM_ALWAYS);
    } else if (Item_TestFrameEqual(item, M_LF_SHOWER_SHOTGUN_PICKUP)) {
        Lara_Mesh_SwapSingle(LM_HAND_R, O_LARA_SHOTGUN);
    } else if (Item_TestFrameEqual(item, -1)) {
        Game_SetIsLevelComplete(true);
    }

    if (Music_GetCurrentPlayingTrack() == Music_ToGameID(MX_CUTSCENE_BATH)) {
        const int32_t frame_num = Item_GetRelativeFrame(item);
        const double ts = (frame_num - M_LF_SHOWER_START) / (double)LOGIC_FPS;
        Music_SyncTimestamp(ts);
    }
}

// clang-format off
REGISTER_LARA_STATE(LS_USE_MIDAS,         M_UseMidas)
REGISTER_LARA_STATE(LS_DIE_MIDAS,         M_DieMidas)
#if TR_VERSION >= 2
REGISTER_LARA_EXTRA(LS_EXTRA_BREATH,      M_Breath)
REGISTER_LARA_EXTRA(LS_EXTRA_YETI_KILL,   M_YetiKill)
REGISTER_LARA_EXTRA(LS_EXTRA_SHARK_KILL,  M_SharkKill)
REGISTER_LARA_EXTRA(LS_EXTRA_AIRLOCK,     M_Airlock)
REGISTER_LARA_EXTRA(LS_EXTRA_GONG_BONG,   M_GongBong)
REGISTER_LARA_EXTRA(LS_EXTRA_TREX_KILL,   M_DinoKill)
REGISTER_LARA_EXTRA(LS_EXTRA_PULL_DAGGER, M_PullDagger)
REGISTER_LARA_EXTRA(LS_EXTRA_START_ANIM,  M_StartAnim)
REGISTER_LARA_EXTRA(LS_EXTRA_START_HOUSE, M_StartHouse)
REGISTER_LARA_EXTRA(LS_EXTRA_FINAL_ANIM,  M_FinalAnim)
#endif
// clang-format on

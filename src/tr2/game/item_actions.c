#include "game/item_actions.h"

#include "game/stats.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/spawn.h>
#include <libtrx/game/viewport.h>
#include <libtrx/utils.h>

typedef void (*M_FUNC)(ITEM *item);

static void M_Bubbles(ITEM *const item)
{
    // XXX: until we get RoboLara, it makes sense for her to breathe underwater
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (lara->water_status == LWS_CHEAT
        && !(Room_Get(lara_item->room_num)->flags & RF_UNDERWATER)) {
        return;
    }

    const int32_t count = (Random_GetDraw() * 3) / 0x8000;
    if (count == 0) {
        return;
    }

    Sound_Effect(SFX_LARA_BUBBLES, &item->pos, SPM_UNDERWATER);

    XYZ_32 offset = { .x = 0, .y = 0, .z = 50 };
    Collide_GetJointAbsPosition(item, &offset, LM_HEAD);
    for (int32_t i = 0; i < count; i++) {
        Spawn_Bubble(&offset, item->room_num);
    }
}

static void M_LaraHandsFree(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_ARMLESS;
}

static void M_FinishLevel(ITEM *const item)
{
    Game_SetIsLevelComplete(true);
}

static void M_Turn180(ITEM *const item)
{
    item->rot.x = -item->rot.x;
    item->rot.y += DEG_180;
}

static void M_FloorShake(ITEM *const item)
{
    const int32_t max_dist = WALL_L * 16; // = 0x4000
    const int32_t max_bounce = 100;

    const int32_t dx = item->pos.x - g_Camera.pos.pos.x;
    const int32_t dy = item->pos.y - g_Camera.pos.pos.y;
    const int32_t dz = item->pos.z - g_Camera.pos.pos.z;
    const int32_t dist = SQUARE(dz) + SQUARE(dy) + SQUARE(dx);

    if (ABS(dx) < max_dist && ABS(dy) < max_dist && ABS(dz) < max_dist) {
        g_Camera.bounce =
            max_bounce * (SQUARE(WALL_L) - dist / 256) / SQUARE(WALL_L);
    }
}

static void M_LaraNormal(ITEM *const item)
{
    item->current_anim_state = LS(LS_STOP);
    item->goal_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(-1);
}

static void M_Boiler(ITEM *const item)
{
    Sound_Effect(SFX_BOILER, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_Flood(ITEM *const item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    if (flip_timer > 4 * LOGIC_FPS) {
        Room_SetFlipEffect(-1);
        Room_IncrementFlipTimer(1);
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    XYZ_32 pos = {
        .x = lara_item->pos.x,
        .y = g_Camera.target.pos.y,
        .z = lara_item->pos.z,
    };
    if (flip_timer >= LOGIC_FPS) {
        pos.y += 100 * (flip_timer - LOGIC_FPS);
    } else {
        pos.y += 100 * (LOGIC_FPS - flip_timer);
    }

    Sound_Effect(SFX_WATERFALL_LOOP, &pos, SPM_NORMAL);
    Room_IncrementFlipTimer(1);
}

static void M_Rubble(ITEM *const item)
{
    Sound_Effect(SFX_MASSIVE_CRASH, nullptr, SPM_NORMAL);
    g_Camera.bounce = -350;
    Room_SetFlipEffect(-1);
}

static void M_Chandelier(ITEM *const item)
{
    const int32_t flip_timer = Room_GetFlipTimer();
    Sound_Effect(SFX_CHAIN_PULLEY, nullptr, SPM_NORMAL);
    if (flip_timer >= LOGIC_FPS) {
        Room_SetFlipEffect(-1);
    }
    Room_IncrementFlipTimer(1);
}

static void M_Explosion(ITEM *const item)
{
    Sound_Effect(SFX_EXPLOSION_1, nullptr, SPM_NORMAL);
    g_Camera.bounce = -75;
    Room_SetFlipEffect(-1);
}

static void M_Piston(ITEM *const item)
{
    Sound_Effect(SFX_PULLEY_CRANE, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_Curtain(ITEM *const item)
{
    Sound_Effect(SFX_CURTAIN, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_Statue(ITEM *const item)
{
    Sound_Effect(SFX_STONE_DOOR_SLIDE, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_SetChange(ITEM *const item)
{
    Sound_Effect(SFX_STAGE_BACKDROP, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_FlipMap(ITEM *const item)
{
    Room_FlipMap();
}

static void M_LaraDrawRightGun(ITEM *const item)
{
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_THIGH_R);
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_HAND_R);
    Lara_Mesh_SwapSingle(LM_THIGH_R, item->object_id);
    Lara_Mesh_SwapSingle(LM_HAND_R, item->object_id);
}

static void M_LaraDrawLeftGun(ITEM *const item)
{
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_THIGH_L);
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_HAND_L);
    Lara_Mesh_SwapSingle(LM_THIGH_L, item->object_id);
    Lara_Mesh_SwapSingle(LM_HAND_L, item->object_id);
}

static void M_SwapMeshesWithMeshSwap1(ITEM *const item)
{
    const OBJECT *const obj_1 = Object_Get(item->object_id);
    for (int32_t mesh_idx = 0; mesh_idx < obj_1->mesh_count; mesh_idx++) {
        Object_SwapMesh(item->object_id, O_MESH_SWAP_1, mesh_idx);
    }
}

static void M_SwapMeshesWithMeshSwap2(ITEM *const item)
{
    const OBJECT *const obj_1 = Object_Get(item->object_id);
    for (int32_t mesh_idx = 0; mesh_idx < obj_1->mesh_count; mesh_idx++) {
        Object_SwapMesh(item->object_id, O_MESH_SWAP_2, mesh_idx);
    }
}

static void M_SwapMeshesWithMeshSwap3(ITEM *const item)
{
    const OBJECT *const obj_1 = Object_Get(item->object_id);
    for (int32_t mesh_idx = 0; mesh_idx < obj_1->mesh_count; mesh_idx++) {
        Object_SwapMesh(item->object_id, O_LARA_SWAP, mesh_idx);
        if (item == Lara_GetItem()) {
            Lara_Mesh_SwapSingle(mesh_idx, item->object_id);
        }
    }
}

static void M_InvisibilityOn(ITEM *const item)
{
    item->status = IS_INVISIBLE;
}

static void M_InvisibilityOff(ITEM *const item)
{
    item->status = IS_ACTIVE;
}

static void M_DynamicLightOn(ITEM *const item)
{
    item->dynamic_light = true;
}

static void M_DynamicLightOff(ITEM *const item)
{
    item->dynamic_light = false;
}

static void M_ResetHair(ITEM *const item)
{
    Lara_Hair_Initialise();
}

static void M_AssaultStart(ITEM *const item)
{
    Gym_StartAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultStop(ITEM *const item)
{
    Gym_StopAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultReset(ITEM *const item)
{
    Gym_ResetAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultFinished(ITEM *const item)
{
    Gym_FinishAssault();
    Room_SetFlipEffect(-1);
}

void ItemAction_Run(int16_t action_id, ITEM *item)
{
    static M_FUNC m_Actions[] = {
        // clang-format off
        [ITEM_ACTION_TURN_180]                     = M_Turn180,
        [ITEM_ACTION_FLOOR_SHAKE]                  = M_FloorShake,
        [ITEM_ACTION_LARA_NORMAL]                  = M_LaraNormal,
        [ITEM_ACTION_BUBBLES]                      = M_Bubbles,
        [ITEM_ACTION_FINISH_LEVEL]                 = M_FinishLevel,
        [ITEM_ACTION_FLOOD]                        = M_Flood,
        [ITEM_ACTION_CHANDELIER]                   = M_Chandelier,
        [ITEM_ACTION_RUBBLE]                       = M_Rubble,
        [ITEM_ACTION_PISTON]                       = M_Piston,
        [ITEM_ACTION_CURTAIN]                      = M_Curtain,
        [ITEM_ACTION_SET_CHANGE]                   = M_SetChange,
        [ITEM_ACTION_EXPLOSION]                    = M_Explosion,
        [ITEM_ACTION_LARA_HANDS_FREE]              = M_LaraHandsFree,
        [ITEM_ACTION_FLIP_MAP]                     = M_FlipMap,
        [ITEM_ACTION_LARA_DRAW_RIGHT_GUN]          = M_LaraDrawRightGun,
        [ITEM_ACTION_LARA_DRAW_LEFT_GUN]           = M_LaraDrawLeftGun,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_1] = M_SwapMeshesWithMeshSwap1,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_2] = M_SwapMeshesWithMeshSwap2,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_3] = M_SwapMeshesWithMeshSwap3,
        [ITEM_ACTION_INVISIBILITY_ON]              = M_InvisibilityOn,
        [ITEM_ACTION_INVISIBILITY_OFF]             = M_InvisibilityOff,
        [ITEM_ACTION_DYNAMIC_LIGHT_ON]             = M_DynamicLightOn,
        [ITEM_ACTION_DYNAMIC_LIGHT_OFF]            = M_DynamicLightOff,
        [ITEM_ACTION_STATUE]                       = M_Statue,
        [ITEM_ACTION_RESET_HAIR]                   = M_ResetHair,
        [ITEM_ACTION_BOILER]                       = M_Boiler,
        [ITEM_ACTION_ASSAULT_RESET]                = M_AssaultReset,
        [ITEM_ACTION_ASSAULT_STOP]                 = M_AssaultStop,
        [ITEM_ACTION_ASSAULT_START]                = M_AssaultStart,
        [ITEM_ACTION_ASSAULT_FINISHED]             = M_AssaultFinished,
        // clang-format on
    };

    if (m_Actions[action_id] != nullptr) {
        m_Actions[action_id](item);
    }
}

void ItemAction_RunActive(void)
{
    const int32_t flip_effect = Room_GetFlipEffect();
    if (flip_effect != -1) {
        ItemAction_Run(flip_effect, nullptr);
    }
}

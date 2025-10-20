#include "game/stats.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/items/actions/ids.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/spawn.h>
#include <libtrx/game/viewport.h>
#include <libtrx/utils.h>

typedef void (*M_FUNC)(ITEM *item);

static void M_FinishLevel(ITEM *const item)
{
    Game_SetIsLevelComplete(true);
}

static void M_Turn180(ITEM *const item)
{
    item->rot.x = -item->rot.x;
    item->rot.y += DEG_180;
    if (item == Lara_GetItem()) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->move_angle += DEG_180;
    }
}

static void M_Boiler(ITEM *const item)
{
    Sound_Effect(SFX_BOILER, nullptr, SPM_NORMAL);
    Room_SetFlipEffect(-1);
}

static void M_FlipMap(ITEM *const item)
{
    Room_FlipMap();
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

void Item_ActionRunLegacy(ITEM_TRX_ACTION action_id, ITEM *item)
{
    static M_FUNC m_Actions[] = {
        // clang-format off
        [ITEM_ACTION_TURN_180]                     = M_Turn180,
        [ITEM_ACTION_FINISH_LEVEL]                 = M_FinishLevel,
        [ITEM_ACTION_FLIP_MAP]                     = M_FlipMap,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_1] = M_SwapMeshesWithMeshSwap1,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_2] = M_SwapMeshesWithMeshSwap2,
        [ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_3] = M_SwapMeshesWithMeshSwap3,
        [ITEM_ACTION_INVISIBILITY_ON]              = M_InvisibilityOn,
        [ITEM_ACTION_INVISIBILITY_OFF]             = M_InvisibilityOff,
        [ITEM_ACTION_DYNAMIC_LIGHT_ON]             = M_DynamicLightOn,
        [ITEM_ACTION_DYNAMIC_LIGHT_OFF]            = M_DynamicLightOff,
        [ITEM_ACTION_BOILER]                       = M_Boiler,
        [ITEM_ACTION_ASSAULT_RESET]                = M_AssaultReset,
        [ITEM_ACTION_ASSAULT_STOP]                 = M_AssaultStop,
        [ITEM_ACTION_ASSAULT_START]                = M_AssaultStart,
        [ITEM_ACTION_ASSAULT_FINISHED]             = M_AssaultFinished,
        // clang-format on
    };

    if (action_id >= 0 && action_id < ITEM_ACTION_NUMBER_OF
        && m_Actions[action_id] != nullptr) {
        m_Actions[action_id](item);
    }
}

#include "game/lara.h"

static void M_Turn180(ITEM *const item)
{
    item->rot.x = -item->rot.x;
    item->rot.y += DEG_180;
    if (item == Lara_GetItem()) {
        LARA_INFO *const lara = Lara_GetLaraInfo();
        lara->move_angle += DEG_180;
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

static void M_SwapMeshes(ITEM *const item, const OBJECT_ID swap_id)
{
    const OBJECT *const obj_1 = Object_Get(item->object_id);
    const bool target_lara = swap_id == O_LARA_SWAP && item == Lara_GetItem();
    for (int32_t mesh_idx = 0; mesh_idx < obj_1->mesh_count; mesh_idx++) {
        Object_SwapMesh(item->object_id, swap_id, mesh_idx);
        if (target_lara) {
            Lara_Mesh_SwapSingle(mesh_idx, item->object_id);
        }
    }
}

static void M_SwapMeshesWithMeshSwap1(ITEM *const item)
{
    M_SwapMeshes(item, O_MESH_SWAP_1);
}

static void M_SwapMeshesWithMeshSwap2(ITEM *const item)
{
    M_SwapMeshes(item, O_MESH_SWAP_2);
}

static void M_SwapMeshesWithMeshSwap3(ITEM *const item)
{
    M_SwapMeshes(item, O_LARA_SWAP);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_TURN_180, M_Turn180)
REGISTER_ITEM_ACTION(ITEM_ACTION_INVISIBILITY_ON, M_InvisibilityOn)
REGISTER_ITEM_ACTION(ITEM_ACTION_INVISIBILITY_OFF, M_InvisibilityOff)
REGISTER_ITEM_ACTION(ITEM_ACTION_DYNAMIC_LIGHT_ON, M_DynamicLightOn)
REGISTER_ITEM_ACTION(ITEM_ACTION_DYNAMIC_LIGHT_OFF, M_DynamicLightOff)
REGISTER_ITEM_ACTION(
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_1, M_SwapMeshesWithMeshSwap1)
REGISTER_ITEM_ACTION(
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_2, M_SwapMeshesWithMeshSwap2)
REGISTER_ITEM_ACTION(
    ITEM_ACTION_SWAP_MESHES_WITH_MESH_SWAP_3, M_SwapMeshesWithMeshSwap3)

#include "game/objects/common.h"

#include "debug.h"
#include "game/anims.h"
#include "game/catalog.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/lara/common.h"
#include "game/objects/vars.h"
#include "game/output/common.h"
#include "memory.h"
#include "vector.h"

#include <string.h>

static OBJECT m_Objects[O_NUMBER_OF] = {};
static STATIC_OBJECT_3D m_StaticObjects3D[MAX_STATIC_OBJECTS_3D] = {};
static STATIC_OBJECT_2D m_StaticObjects2D[MAX_STATIC_OBJECTS_2D] = {};
static OBJECT_MESH **m_MeshPointers = nullptr;
static int32_t m_MeshCount = 0;

void Object_Reset(void)
{
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        m_Objects[i].loaded = false;
    }
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS_3D; i++) {
        m_StaticObjects3D[i].loaded = false;
    }
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS_2D; i++) {
        m_StaticObjects2D[i].loaded = false;
    }
}

OBJECT *Object_TryGet(const OBJECT_ID object_id)
{
    if (object_id < O_FIRST || object_id >= O_NUMBER_OF) {
        return nullptr;
    }
    return &m_Objects[object_id];
}

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    ASSERT(object_id >= O_FIRST && object_id < O_NUMBER_OF);
    return &m_Objects[object_id];
}

OBJECT *Object_GetByGameID(const int32_t game_id)
{
    OBJECT_ID object_id = Object_FromGameID(game_id);
    if (object_id == NO_OBJECT) {
        return nullptr;
    }
    return &m_Objects[object_id];
}

STATIC_OBJECT_3D *Object_Get3DStatic(const int32_t static_id)
{
    return &m_StaticObjects3D[static_id];
}

STATIC_OBJECT_2D *Object_Get2DStatic(const int32_t static_id)
{
    return &m_StaticObjects2D[static_id];
}

OBJECT_ID Object_FromGameID(const int32_t game_id)
{
    int32_t out;
    if (Catalog_GameIDToEnum(CATALOG_OBJECTS, game_id, &out)) {
        return out;
    }
    return NO_OBJECT;
}

int32_t Object_ToGameID(const OBJECT_ID object_id)
{
    int32_t out;
    if (Catalog_EnumToGameID(CATALOG_OBJECTS, object_id, &out)) {
        return out;
    }
    return -1;
}

bool Object_IsType(const OBJECT_ID object_id, const OBJECT_ID *test_arr)
{
    for (int32_t i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

OBJECT_ID Object_GetCognate(OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->key_id == key_id) {
            return pair->value_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

OBJECT_ID Object_GetCognateInverse(
    OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->value_id == value_id) {
            return pair->key_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

void Object_InitialiseMeshes(const int32_t mesh_count)
{
    m_MeshPointers =
        GameBuf_Alloc(sizeof(OBJECT_MESH *) * mesh_count, GBUF_MESH_POINTERS);
    m_MeshCount = 0;
}

void Object_StoreMesh(OBJECT_MESH *const mesh)
{
    m_MeshPointers[m_MeshCount] = mesh;
    m_MeshCount++;
}

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    return m_MeshPointers[index];
}

int32_t Object_GetMeshIndex(const OBJECT_MESH *const mesh)
{
    for (int32_t i = 0; i < m_MeshCount; i++) {
        if (mesh == m_MeshPointers[i]) {
            return i;
        }
    }
    return -1;
}

int32_t Object_GetMeshCount(void)
{
    return m_MeshCount;
}

OBJECT_MESH *Object_FindMesh(const int32_t data_offset)
{
    for (int32_t i = 0; i < m_MeshCount; i++) {
        OBJECT_MESH *const mesh = m_MeshPointers[i];
        if (Object_GetMeshOffset(mesh) == data_offset) {
            return mesh;
        }
    }

    return nullptr;
}

int32_t Object_GetMeshOffset(const OBJECT_MESH *const mesh)
{
    return (int32_t)(intptr_t)mesh->priv;
}

void Object_SetMeshOffset(OBJECT_MESH *const mesh, const int32_t data_offset)
{
    mesh->priv = (void *)(intptr_t)data_offset;
}

void Object_SwapMesh(
    const OBJECT_ID object1_id, const OBJECT_ID object2_id,
    const int32_t mesh_num)
{
    const OBJECT *const obj1 = Object_Get(object1_id);
    const OBJECT *const obj2 = Object_Get(object2_id);

    SWAP(
        m_MeshPointers[obj1->mesh_idx + mesh_num],
        m_MeshPointers[obj2->mesh_idx + mesh_num]);

    Output_DispatchObjectMeshSwap(
        obj1->mesh_idx + mesh_num, obj2->mesh_idx + mesh_num);
}

ANIM *Object_GetAnim(const OBJECT *const obj, const int32_t anim_idx)
{
    return Anim_GetAnim(obj->anim_idx + anim_idx);
}

ANIM_BONE *Object_GetBone(const OBJECT *const obj, const int32_t bone_idx)
{
    return Anim_GetBone(obj->bone_idx + bone_idx);
}

OBJECT_ID Object_FindReceptacleKey(const OBJECT_ID receptacle_obj_id)
{
    return Object_GetCognateInverse(
        receptacle_obj_id, g_KeyItemToReceptacleMap);
}

int16_t Object_FindReceptacle(const OBJECT_ID object_id)
{
    // Iterate through all matching receptacles
    const GAME_OBJECT_PAIR *const map = g_KeyItemToReceptacleMap;
    for (int32_t i = 0; map[i].key_id != NO_OBJECT; i++) {
        if (map[i].key_id != object_id) {
            continue;
        }

        // Iterate through all level items that match this receptacle
        const OBJECT_ID receptacle_to_check = map[i].value_id;
        for (int16_t item_num = 0; item_num < Item_GetLevelCount();
             item_num++) {
            const ITEM *const item = Item_Get(item_num);
            if (item->object_id != receptacle_to_check) {
                continue;
            }

            const OBJECT *const obj = Object_Get(item->object_id);
            if (obj->is_usable_func != nullptr
                && !obj->is_usable_func(item_num)) {
                continue;
            }

            // If Lara is standing near one, that's our keyhole
            if (Lara_TestPosition(item, obj->bounds_func())) {
                return item_num;
            }
        }
    }

    return NO_ITEM;
}

bool Object_CanInterpolate(
    const ITEM *const item, const int32_t frame_a, const int32_t frame_b)
{
    return item->active && item->status == IS_ACTIVE
        && item->enable_interpolation
        && Object_Get(item->object_id)->enable_interpolation;
}

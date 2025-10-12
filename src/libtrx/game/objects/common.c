#include "game/objects/common.h"

#include "debug.h"
#include "game/anims.h"
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

static const char *m_ObjectUUIDStrings[O_NUMBER_OF] = {
#define X_OBJ_ID_DEFINE(uuid_str, enum_value, game1_id, game2_id)              \
    [enum_value] = uuid_str,
#include "game/objects/ids.def"
#undef X_OBJ_ID_DEFINE
};

static int32_t m_GameIDMap[TR_VERSION_COUNT][O_NUMBER_OF] = {
    [0] = {
#define X_OBJ_ID_DEFINE(_u, ev, g1, g2) [ev] = (g1),
#include "game/objects/ids.def"
#undef X_OBJ_ID_DEFINE
    },
    [1] = {
#define X_OBJ_ID_DEFINE(_u, ev, g1, g2) [ev] = (g2),
#include "game/objects/ids.def"
#undef X_OBJ_ID_DEFINE
    }
};

static VECTOR *m_GameIDReverseMap[TR_VERSION_COUNT] = {};

static bool M_HexCharValue(const char c, uint8_t *const val)
{
    if (c >= '0' && c <= '9') {
        *val = c - '0';
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *val = c - 'a' + 10;
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *val = c - 'A' + 10;
        return true;
    }
    return false;
}

static void M_ParseUUIDString(const char *str, UUID *const uuid_out)
{
    if (str == nullptr || *str == '\0') {
        memset(uuid_out->bytes, 0, 16);
        return;
    }

    // UUID field sizes
    const int32_t field_sizes[] = { 4, 2, 2, 2, 6 };
    uint8_t *byte_ptr = uuid_out->bytes;

    for (int32_t field = 0; field < 5; field++) {
        const int32_t num_bytes = field_sizes[field];

        for (int32_t i = 0; i < num_bytes; i++) {
            while (*str == '-') {
                str++;
            }

            uint8_t hi, lo;
            if (!M_HexCharValue(*str++, &hi) || !M_HexCharValue(*str++, &lo)) {
                memset(uuid_out->bytes, 0, 16);
                return;
            }

            // For fields 0, 1, and 2, handle endianness swap
            if (field < 3) {
                byte_ptr[num_bytes - 1 - i] = (hi << 4) | lo;
            } else {
                byte_ptr[i] = (hi << 4) | lo;
            }
        }

        // Move the pointer forward for each field size
        byte_ptr += num_bytes;
    }
}

__attribute__((constructor)) static void M_InitUUIDs(void)
{
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        M_ParseUUIDString(m_ObjectUUIDStrings[i], &m_Objects[i].uuid);
    }
}

__attribute__((constructor)) static void M_InitGameIDReverseMap(void)
{
    for (int32_t tr_version = 0; tr_version < TR_VERSION_COUNT; tr_version++) {
        int32_t max_game_id = -1;
        for (GAME_OBJECT_ID object_id = O_FIRST; object_id < O_NUMBER_OF;
             object_id++) {
            const int32_t game_id = m_GameIDMap[tr_version][object_id];
            if (game_id > max_game_id) {
                max_game_id = game_id;
            }
        }

        int32_t size = max_game_id + 1;
        m_GameIDReverseMap[tr_version] =
            Vector_CreateAtCapacity(sizeof(GAME_OBJECT_ID), size);

        // Initialize all entries to NO_OBJECT
        GAME_OBJECT_ID *const map_data =
            Vector_GetData(m_GameIDReverseMap[tr_version]);
        for (int32_t i = 0; i < size; i++) {
            map_data[i] = NO_OBJECT;
        }
        // Populate reverse lookup from game ID to object ID
        for (GAME_OBJECT_ID object_id = O_FIRST; object_id < O_NUMBER_OF;
             object_id++) {
            const int32_t game_id = m_GameIDMap[tr_version][object_id];
            if (game_id >= 0) {
                map_data[game_id] = object_id;
            }
        }
        m_GameIDReverseMap[tr_version]->count = size;
    }
}

__attribute__((destructor)) static void M_FreeGameIDReverseMap(void)
{
    for (int32_t tr_version = 0; tr_version < TR_VERSION_COUNT; tr_version++) {
        Vector_Free(m_GameIDReverseMap[tr_version]);
        m_GameIDReverseMap[tr_version] = nullptr;
    }
}

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
    M_InitUUIDs();
}

OBJECT *Object_TryGet(const GAME_OBJECT_ID object_id)
{
    if (object_id < O_FIRST || object_id >= O_NUMBER_OF) {
        return nullptr;
    }
    return &m_Objects[object_id];
}

OBJECT *Object_Get(const GAME_OBJECT_ID object_id)
{
    ASSERT(object_id >= O_FIRST && object_id < O_NUMBER_OF);
    return &m_Objects[object_id];
}

OBJECT *Object_GetByGameID(const int32_t game_id)
{
    GAME_OBJECT_ID object_id = Object_UnmapGameID(game_id);
    if (object_id == NO_OBJECT) {
        return nullptr;
    }
    return &m_Objects[object_id];
}

OBJECT *Object_GetByUUID(const UUID uuid)
{
    GAME_OBJECT_ID object_id = Object_UnmapUUID(uuid);
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

GAME_OBJECT_ID Object_UnmapGameID(const int32_t game_id)
{
    const int ver = TR_VERSION - 1;
    VECTOR *vec = m_GameIDReverseMap[ver];
    if (vec == nullptr || game_id < 0 || game_id >= vec->count) {
        return NO_OBJECT;
    }
    GAME_OBJECT_ID *map_data = Vector_GetData(vec);
    return map_data[game_id];
}

GAME_OBJECT_ID Object_UnmapUUID(const UUID uuid)
{
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        if (memcmp(&m_Objects[i].uuid, &uuid, sizeof(UUID)) == 0) {
            return i;
        }
    }
    return NO_OBJECT;
}

int32_t Object_MakeGameID(const GAME_OBJECT_ID object_id)
{
    if (object_id < O_FIRST || object_id >= O_NUMBER_OF) {
        return -1;
    }
    return m_GameIDMap[TR_VERSION - 1][object_id];
}

bool Object_IsType(
    const GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr)
{
    for (int32_t i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map)
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

GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map)
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
    const GAME_OBJECT_ID object1_id, const GAME_OBJECT_ID object2_id,
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

GAME_OBJECT_ID Object_FindReceptacleKey(const GAME_OBJECT_ID receptacle_obj_id)
{
    return Object_GetCognateInverse(
        receptacle_obj_id, g_KeyItemToReceptacleMap);
}

int16_t Object_FindReceptacle(const GAME_OBJECT_ID object_id)
{
    // Iterate through all matching receptacles
    const GAME_OBJECT_PAIR *const map = g_KeyItemToReceptacleMap;
    for (int32_t i = 0; map[i].key_id != NO_OBJECT; i++) {
        if (map[i].key_id != object_id) {
            continue;
        }

        // Iterate through all level items that match this receptacle
        const GAME_OBJECT_ID receptacle_to_check = map[i].value_id;
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

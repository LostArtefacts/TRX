#include "game/objects/common.h"

#include "debug.h"
#include "game/anims.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/output/objects.h"

#include <string.h>

static OBJECT m_Objects[O_NUMBER_OF] = {};
static STATIC_OBJECT_3D m_StaticObjects3D[MAX_STATIC_OBJECTS_3D] = {};
static STATIC_OBJECT_2D m_StaticObjects2D[MAX_STATIC_OBJECTS_2D] = {};
static OBJECT_MESH **m_MeshPointers = nullptr;
static int32_t m_MeshCount = 0;
static bool m_UUIDsParsed = false;

static const char *m_ObjectUUIDStrings[O_NUMBER_OF] = {
#undef OBJ_ID_DEFINE
#define OBJ_ID_ALIAS_DEFINE(source_id, target_id)
#define OBJ_ID_DEFINE(game_id, uuid_str, enum_value) [game_id] = uuid_str,
#include "game/objects/ids.def"
#undef OBJ_ID_DEFINE
#undef OBJ_ID_ALIAS_DEFINE
};

static bool M_HexCharValue(char c, uint8_t *val);
static void M_ParseUUIDString(const char *str, UUID *uuid_out);
static void M_InitUUIDs(void);

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

static void M_InitUUIDs(void)
{
    if (m_UUIDsParsed) {
        return;
    }
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        M_ParseUUIDString(m_ObjectUUIDStrings[i], &m_Objects[i].uuid);
    }
    m_UUIDsParsed = true;
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
    m_UUIDsParsed = false;
}

OBJECT *Object_Get(const GAME_OBJECT_ID object_id)
{
    ASSERT(object_id >= O_FIRST && object_id < O_NUMBER_OF);
    return &m_Objects[object_id];
}

OBJECT *Object_GetByGameID(const int32_t game_id)
{
    GAME_OBJECT_ID obj_id = Object_UnmapGameID(game_id);
    if (obj_id == NO_OBJECT) {
        return nullptr;
    }
    return &m_Objects[obj_id];
}

OBJECT *Object_GetByUUID(const UUID uuid)
{
    GAME_OBJECT_ID obj_id = Object_UnmapUUID(uuid);
    if (obj_id == NO_OBJECT) {
        return nullptr;
    }
    return &m_Objects[obj_id];
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
    if (game_id < O_FIRST || game_id >= O_NUMBER_OF) {
        return NO_OBJECT;
    }
    return game_id + O_FIRST;
}

GAME_OBJECT_ID Object_UnmapUUID(const UUID uuid)
{
    if (!m_UUIDsParsed) {
        M_InitUUIDs();
    }
    for (int32_t i = O_FIRST; i < O_NUMBER_OF; i++) {
        if (memcmp(&m_Objects[i].uuid, &uuid, sizeof(UUID)) == 0) {
            return i;
        }
    }
    return NO_OBJECT;
}

int32_t Object_MakeGameID(const GAME_OBJECT_ID game_id)
{
    return game_id - O_FIRST;
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

    OBJECT_MESH *const temp = m_MeshPointers[obj1->mesh_idx + mesh_num];
    m_MeshPointers[obj1->mesh_idx + mesh_num] =
        m_MeshPointers[obj2->mesh_idx + mesh_num];
    m_MeshPointers[obj2->mesh_idx + mesh_num] = temp;

#if TR_VERSION == 1
    Output_Meshes_ObserveObjectMeshSwap(
        m_MeshPointers[obj1->mesh_idx + mesh_num],
        m_MeshPointers[obj2->mesh_idx + mesh_num]);
#endif
}

ANIM *Object_GetAnim(const OBJECT *const obj, const int32_t anim_idx)
{
    return Anim_GetAnim(obj->anim_idx + anim_idx);
}

ANIM_BONE *Object_GetBone(const OBJECT *const obj, const int32_t bone_idx)
{
    return Anim_GetBone(obj->bone_idx + bone_idx);
}

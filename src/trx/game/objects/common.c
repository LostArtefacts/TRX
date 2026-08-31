#include <trx/game/objects/common.h>

#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/anims.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/catalog/table.h>
#include <trx/game/const.h>
#include <trx/game/game_buf.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects/links.h>
#include <trx/game/output/common.h>

typedef struct {
    OBJECT_SLOT slot_num;
    OBJECT obj;
} M_UNCATALOGED_SLOT;

CATALOG_TABLE_DEFINE(m_Objects, CATALOG_OBJECTS, OBJECT);
static STATIC_OBJECT_3D *m_StaticObjects3D = nullptr;
static STATIC_OBJECT_2D *m_StaticObjects2D = nullptr;
static int32_t m_StaticObjects3DCount = 0;
static int32_t m_StaticObjects2DCount = 0;
static VECTOR *m_MeshPointers = nullptr;

static VECTOR *m_UncatalogedSlots = nullptr;

void Object_Reset(void)
{
    CATALOG_FOR_EACH(CATALOG_OBJECTS, i)
    {
        OBJECT *const obj = Object_TryGet(i);
        ObjectProperty_ResetObject(obj);
        obj->loaded = false;
    }

    m_StaticObjects3D = nullptr;
    m_StaticObjects2D = nullptr;
    m_StaticObjects3DCount = 0;
    m_StaticObjects2DCount = 0;
    if (m_MeshPointers != nullptr) {
        Vector_Free(m_MeshPointers);
        m_MeshPointers = nullptr;
    }

    if (m_UncatalogedSlots != nullptr) {
        Vector_Free(m_UncatalogedSlots);
        m_UncatalogedSlots = nullptr;
    }

    CatalogTable_Free(&m_Objects);
}

void Object_InitialiseStaticObjects3D(const int32_t count)
{
    ASSERT(count >= 0);
    m_StaticObjects3DCount = count;
    m_StaticObjects3D =
        GameBuf_Alloc(sizeof(STATIC_OBJECT_3D) * count, GBUF_STATIC_OBJECTS_3D);
}

void Object_InitialiseStaticObjects2D(const int32_t count)
{
    ASSERT(count >= 0);
    m_StaticObjects2DCount = count;
    m_StaticObjects2D =
        GameBuf_Alloc(sizeof(STATIC_OBJECT_2D) * count, GBUF_STATIC_OBJECTS_2D);
}

int32_t Object_GetStaticObjects3DCount(void)
{
    return m_StaticObjects3DCount;
}

int32_t Object_GetStaticObjects2DCount(void)
{
    return m_StaticObjects2DCount;
}

void Object_Register(
    const OBJECT_ID object_id, void (*const setup_func)(OBJECT *obj))
{
    OBJECT *const obj = CatalogTable_Claim(&m_Objects, object_id);
    obj->setup_func = setup_func;
}

OBJECT *Object_TryGet(const OBJECT_ID object_id)
{
    return CatalogTable_TryGet(&m_Objects, object_id);
}

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    OBJECT *const obj = Object_TryGet(object_id);
    ASSERT(obj != nullptr);
    return obj;
}

OBJECT_ID Object_Mint(void)
{
    // Register the object so the object tables recognise its id. It takes no
    // name, which keeps it out of savegames.
    return Catalog_CreateAnonymous(CATALOG_OBJECTS);
}

int32_t Object_GetCount(void)
{
    return Catalog_GetCount(CATALOG_OBJECTS);
}

OBJECT *Object_GetBySlot(const OBJECT_SLOT slot)
{
    OBJECT_ID object_id = Object_SlotToID(slot);
    if (object_id == NO_OBJECT) {
        return nullptr;
    }
    return Object_TryGet(object_id);
}

void Object_StoreUncatalogedSlot(
    const OBJECT_SLOT slot_num, const OBJECT *const obj)
{
    if (m_UncatalogedSlots == nullptr) {
        m_UncatalogedSlots = Vector_Create(sizeof(M_UNCATALOGED_SLOT));
    }
    const M_UNCATALOGED_SLOT slot = { .slot_num = slot_num, .obj = *obj };
    Vector_Add(m_UncatalogedSlots, &slot);
}

const OBJECT *Object_GetUncatalogedSlot(const OBJECT_SLOT slot_num)
{
    if (m_UncatalogedSlots == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; i < m_UncatalogedSlots->count; i++) {
        const M_UNCATALOGED_SLOT *const slot =
            Vector_Get(m_UncatalogedSlots, i);
        if (slot->slot_num == slot_num) {
            return &slot->obj;
        }
    }
    return nullptr;
}

STATIC_OBJECT_3D *Object_Get3DStatic(const int32_t static_id)
{
    return &m_StaticObjects3D[static_id];
}

bool Object_IsValidStatid3D(const int32_t static_id)
{
    return static_id >= 0 && static_id < m_StaticObjects3DCount;
}

STATIC_OBJECT_2D *Object_Get2DStatic(const int32_t static_id)
{
    if (m_StaticObjects2D == nullptr) {
        return nullptr;
    }
    if (static_id < 0 || static_id >= m_StaticObjects2DCount) {
        return nullptr;
    }
    return &m_StaticObjects2D[static_id];
}

OBJECT_ID Object_SlotToID(const OBJECT_SLOT slot)
{
    return Catalog_SlotToID(CATALOG_OBJECTS, slot, NO_OBJECT);
}

OBJECT_SLOT Object_IDToSlot(const OBJECT_ID id)
{
    return Catalog_IDToSlot(CATALOG_OBJECTS, id, -1);
}

void Object_InitialiseMeshes(const int32_t mesh_count)
{
    if (m_MeshPointers != nullptr) {
        Vector_Free(m_MeshPointers);
    }
    m_MeshPointers = Vector_CreateAtCapacity(sizeof(OBJECT_MESH *), mesh_count);
}

void Object_StoreMesh(OBJECT_MESH *const mesh)
{
    Vector_Add(m_MeshPointers, (void *)&mesh);
}

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    return *(OBJECT_MESH **)Vector_Get(m_MeshPointers, index);
}

int32_t Object_GetItemMeshIndex(const ITEM *const item, const int32_t mesh_idx)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const int32_t fallback = obj->mesh_idx + mesh_idx;

    if (obj->get_mesh_index_func == nullptr) {
        return fallback;
    }

    const int32_t resolved = obj->get_mesh_index_func(item, mesh_idx);
    if (resolved < 0) {
        return fallback;
    }

    return resolved;
}

int32_t Object_GetMeshIndex(const OBJECT_MESH *const mesh)
{
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        if (mesh == Object_GetMesh(i)) {
            return i;
        }
    }
    return -1;
}

int32_t Object_GetMeshCount(void)
{
    return m_MeshPointers == nullptr ? 0 : m_MeshPointers->count;
}

OBJECT_MESH *Object_FindMesh(const int32_t data_offset)
{
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        OBJECT_MESH *const mesh = Object_GetMesh(i);
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
    Object_SwapMeshEx(object1_id, object2_id, mesh_num, mesh_num);
}

void Object_SwapAllMeshes(
    const OBJECT_ID object1_id, const OBJECT_ID object2_id)
{
    const OBJECT *const obj1 = Object_Get(object1_id);
    const OBJECT *const obj2 = Object_Get(object2_id);
    if (!obj1->loaded || !obj2->loaded) {
        return;
    }

    const int32_t mesh_count = MIN(obj1->mesh_count, obj2->mesh_count);
    for (int32_t i = 0; i < mesh_count; i++) {
        Object_SwapMeshEx(object1_id, object2_id, i, i);
    }
}

void Object_SwapMeshEx(
    const OBJECT_ID object1_id, const OBJECT_ID object2_id,
    const int32_t mesh_num1, const int32_t mesh_num2)
{
    const OBJECT *const obj1 = Object_Get(object1_id);
    const OBJECT *const obj2 = Object_Get(object2_id);
    if (!obj1->loaded || !obj2->loaded) {
        return;
    }

    const int32_t mesh_idx1 = obj1->mesh_idx + mesh_num1;
    const int32_t mesh_idx2 = obj2->mesh_idx + mesh_num2;
    OBJECT_MESH **const slot_1 = Vector_Get(m_MeshPointers, mesh_idx1);
    OBJECT_MESH **const slot_2 = Vector_Get(m_MeshPointers, mesh_idx2);
    SWAP(*slot_1, *slot_2);

    Output_DispatchObjectMeshSwap(mesh_idx1, mesh_idx2);
}

void Object_SwapSprite(const OBJECT_ID object1_id, const OBJECT_ID object2_id)
{
    OBJECT *const obj1 = Object_Get(object1_id);
    OBJECT *const obj2 = Object_Get(object2_id);
    if (!obj1->loaded || !obj2->loaded) {
        return;
    }

    // A sprite object keeps its sprite where a modelled one keeps its meshes:
    // the first frame and how many there are. Nothing is dispatched, as the
    // sprite is looked up by index as it is drawn.
    SWAP(obj1->mesh_idx, obj2->mesh_idx);
    SWAP(obj1->mesh_count, obj2->mesh_count);
}

ANIM *Object_GetAnim(const OBJECT *const obj, const int32_t anim_idx)
{
    return Anim_GetAnim(obj->anim_idx + anim_idx);
}

ANIM_BONE *Object_GetBone(const OBJECT *const obj, const int32_t bone_idx)
{
    return Anim_GetBone(obj->bone_idx + bone_idx);
}

ANIM_BONE *Object_TryGetBone(const OBJECT *const obj, const int32_t bone_idx)
{
    return Anim_TryGetBone(obj->bone_idx + bone_idx);
}

OBJECT_ID Object_FindReceptacleKey(const OBJECT_ID receptacle_obj_id)
{
    return ObjectLink_GetInverse(receptacle_obj_id, OBJ_LINK_KEY_TO_RECEPTACLE);
}

int16_t Object_FindReceptacle(const OBJECT_ID object_id)
{
    // Iterate through all matching receptacles
    const int32_t count =
        ObjectLink_GetCount(object_id, OBJ_LINK_KEY_TO_RECEPTACLE);
    for (int32_t i = 0; i < count; i++) {
        // Iterate through all level items that match this receptacle
        const OBJECT_ID receptacle_to_check =
            ObjectLink_GetAt(object_id, OBJ_LINK_KEY_TO_RECEPTACLE, i);
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
    const OBJECT *const obj = Object_Get(item->object_id);
    return item->enable_interpolation && obj->enable_interpolation
        && item->prev_frame_num != item->frame_num;
}

void Object_SetReflective(const OBJECT_ID obj_id, const bool enabled)
{
    const OBJECT *const obj = Object_Get(obj_id);
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        Object_SetMeshReflective(obj_id, i, enabled);
    }
}

void Object_SetMeshReflective(
    const OBJECT_ID obj_id, const int32_t mesh_idx, const bool enabled)
{
    const OBJECT *const obj = Object_Get(obj_id);
    if (!obj->loaded) {
        return;
    }

    const int32_t abs_mesh_idx = obj->mesh_idx + mesh_idx;
    OBJECT_MESH *const mesh = Object_GetMesh(abs_mesh_idx);
    mesh->enable_reflections = enabled;
    for (int32_t i = 0; i < mesh->all_faces.count; i++) {
        mesh->all_faces.data[i].enable_reflections = enabled;
    }
    Output_DispatchObjectMeshUpdate(abs_mesh_idx);
}

void Object_SetSemiTransparent(const OBJECT_ID obj_id, const bool enabled)
{
    const OBJECT *const obj = Object_Get(obj_id);
    if (!obj->loaded) {
        return;
    }
    for (int32_t i = 0; i < obj->mesh_count; i++) {
        OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx + i);
        for (int32_t j = 0; j < mesh->all_faces.count; j++) {
            mesh->all_faces.data[j].semi_transparent = enabled;
        }
    }
}

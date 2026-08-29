#include <trx/game/inventory_ring/vars.h>

#include <trx/core/json/util/file.h>
#include <trx/core/memory.h>
#include <trx/core/result.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/shell.h>

INVENTORY_MODE g_InvRing_Mode = INV_TITLE_MODE;
CAMERA_INFO g_InvRing_OldCamera = {};
VECTOR *g_InvRing_Items = nullptr;
INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF] = {};

static void M_Init(void)
{
    g_InvRing_Items = Vector_Create(sizeof(INVENTORY_ITEM *));
}

static void M_Shutdown(void)
{
    if (g_InvRing_Items != nullptr) {
        for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
            INVENTORY_ITEM *const item =
                *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
            Memory_Free(item);
        }
        Vector_Free(g_InvRing_Items);
        g_InvRing_Items = nullptr;
    }
}

static RESULT M_ReadItems(JSON_ARRAY *const arr, const char *const path)
{
#define L_READ_INT(key, target) target = JSON_ObjectGetInt(obj, key, target);

    ASSERT(g_InvRing_Items != nullptr);

    for (size_t i = 0; i < arr->length; i++) {
        JSON_OBJECT *const obj = JSON_ArrayGetObject(arr, i);
        const char *const name =
            JSON_ObjectGetString(obj, "object_id", JSON_INVALID_STRING);
        const CATALOG_ID id = Catalog_KeyToID(CATALOG_OBJECTS, name, NO_OBJECT);
        FAIL_IF(id == NO_OBJECT, "%s: unknown object_id '%s'", path, name);
        INVENTORY_ITEM *const item = Memory_Alloc(sizeof(*item));
        item->object_id = id;
        L_READ_INT("frames_total", item->frames_total);
        L_READ_INT("current_frame", item->current_frame);
        L_READ_INT("goal_frame", item->goal_frame);
        L_READ_INT("open_frame", item->open_frame);
        L_READ_INT("anim_direction", item->anim_direction);
        L_READ_INT("anim_speed", item->anim_speed);
        L_READ_INT("anim_count", item->anim_count);
        L_READ_INT("x_rot_pt_sel", item->x_rot_pt_sel);
        L_READ_INT("x_rot_pt", item->x_rot_pt);
        L_READ_INT("x_rot_sel", item->x_rot_sel);
        L_READ_INT("x_rot_nosel", item->x_rot_nosel);
        L_READ_INT("x_rot", item->x_rot);
        L_READ_INT("y_rot_sel", item->y_rot_sel);
        L_READ_INT("y_rot", item->y_rot);
        L_READ_INT("y_trans_sel", item->y_trans_sel);
        L_READ_INT("y_trans", item->y_trans);
        L_READ_INT("z_trans_sel", item->z_trans_sel);
        L_READ_INT("z_trans", item->z_trans);
        L_READ_INT("meshes_sel", item->meshes_sel);
        L_READ_INT("meshes_drawn", item->meshes_drawn);
        L_READ_INT("inv_pos", item->inv_pos);
        Vector_Add(g_InvRing_Items, &item);
    }

    return OK;
#undef L_READ_INT
}

static RESULT M_LoadFrom(const char *const path)
{
    for (int32_t i = 0; i < g_InvRing_Items->count; i++) {
        INVENTORY_ITEM *const item =
            *(INVENTORY_ITEM **)Vector_Get(g_InvRing_Items, i);
        Memory_Free(item);
    }
    Vector_Clear(g_InvRing_Items);
    for (int32_t i = 0; i < RT_NUMBER_OF; i++) {
        g_InvRing_Source[i].count = 0;
    }

    JSON_VALUE *root = nullptr;
    MUST(JSONFile_ReadRequired(path, &root));
    JSON_ARRAY *const arr = JSON_ValueAsArray(root);
    const RESULT result = arr == nullptr
        ? FAIL("%s: the file must hold a list", path)
        : M_ReadItems(arr, path);
    JSON_ValueFree(root);
    return result;
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    RESULT result = GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "inv_ring.json5", &path);
    if (IS_OK(result)) {
        result = M_LoadFrom(path);
    }
    return result;
}

REGISTER_SUBSYSTEM(.init = M_Init, .load = M_Load, .shutdown = M_Shutdown)

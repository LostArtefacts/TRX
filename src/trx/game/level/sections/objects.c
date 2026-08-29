#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/inject.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/objects.h>
#include <trx/game/shell.h>

static void M_ReadPosition(XYZ_32 *const pos, TRX_FILE *const file)
{
    pos->x = File_ReadS32(file);
    pos->y = File_ReadS32(file);
    pos->z = File_ReadS32(file);
}

static void M_ReadShade(
    const LEVEL_FORMAT_LOADER *const loader, SHADE *const shade,
    TRX_FILE *const file)
{
    shade->value_1 = File_ReadS16(file);
    if (loader->game_version == 1) {
        shade->value_2 = shade->value_1;
    } else {
        shade->value_2 = File_ReadS16(file);
    }
}

static void M_ReadBounds16(BOUNDS_16 *const bounds, TRX_FILE *const file)
{
    bounds->min.x = File_ReadS16(file);
    bounds->max.x = File_ReadS16(file);
    bounds->min.y = File_ReadS16(file);
    bounds->max.y = File_ReadS16(file);
    bounds->min.z = File_ReadS16(file);
    bounds->max.z = File_ReadS16(file);
}

RESULT Level_Section_ReadObjects(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;
    const int32_t num_objects = File_ReadCountS32(file);
    LOG_INFO("objects: %d", num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        OBJECT spare_obj = {};
        const int32_t game_obj_id = File_ReadS32(file);
        OBJECT *obj = Object_GetBySlot(game_obj_id);
        const bool is_cataloged = obj != nullptr;
        if (!is_cataloged) {
            if (loader->game_version == 3 || loader->game_version == 4) {
                // Direct-level support can encounter slots that do not have a
                // stable TRX catalog entry yet.
                obj = &spare_obj;
            } else {
                result = FAIL("invalid object id %d", game_obj_id);
                goto finish;
            }
        }
        obj->mesh_count = File_ReadS16(file);
        obj->mesh_idx = File_ReadS16(file);
        obj->bone_idx = File_ReadS32(file) / ANIM_BONE_SIZE;
        obj->frame_ofs = File_ReadU32(file);
        obj->frame_base = nullptr;
        obj->anim_idx = File_ReadS16(file);
        obj->loaded = true;
        if (!is_cataloged) {
            Object_StoreUncatalogedSlot(game_obj_id, obj);
        }
    }

finish:
    Benchmark_End(&benchmark, nullptr);
    return result;
}

RESULT Level_Section_ReadStaticObjects(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = File_ReadCountS32(file);
    LOG_INFO("static objects: %d", num_objects);

    typedef struct {
        int32_t static_id;
        int16_t mesh_idx;
        BOUNDS_16 draw_bounds;
        BOUNDS_16 collision_bounds;
        uint16_t flags;
    } M_STATIC_OBJ_3D_TEMP;

    M_STATIC_OBJ_3D_TEMP *tmp_statics =
        Memory_Alloc(sizeof(M_STATIC_OBJ_3D_TEMP) * num_objects);

    int32_t max_static_id = -1;
    for (int32_t i = 0; i < num_objects; i++) {
        tmp_statics[i].static_id = File_ReadS32(file);
        if (tmp_statics[i].static_id < 0) {
            result = FAIL("invalid static id %d", tmp_statics[i].static_id);
            goto finish;
        }
        max_static_id = MAX(max_static_id, tmp_statics[i].static_id);

        tmp_statics[i].mesh_idx = File_ReadS16(file);
        M_ReadBounds16(&tmp_statics[i].draw_bounds, file);
        M_ReadBounds16(&tmp_statics[i].collision_bounds, file);
        tmp_statics[i].flags = File_ReadU16(file);
    }

    LOG_INFO("max static id: %d", max_static_id);
    int32_t injection_max_id = Inject_GetMaxStaticObject3DId();
    if (injection_max_id < 0) {
        injection_max_id = -1;
    }
    const int32_t capacity = MAX(max_static_id, injection_max_id) + 1;
    Object_InitialiseStaticObjects3D(capacity);

    for (int32_t i = 0; i < num_objects; i++) {
        STATIC_OBJECT_3D *const obj =
            Object_Get3DStatic(tmp_statics[i].static_id);
        obj->mesh_idx = tmp_statics[i].mesh_idx;
        obj->loaded = true;
        obj->draw_bounds = tmp_statics[i].draw_bounds;
        obj->collision_bounds = tmp_statics[i].collision_bounds;

        obj->collidable = (tmp_statics[i].flags & 1) == 0;
        obj->visible = (tmp_statics[i].flags & 2) != 0;

        Object_GetMesh(obj->mesh_idx)->enable_caustics = obj->visible;
    }

finish:
    Memory_FreePointer(&tmp_statics);
    Benchmark_End(&benchmark, nullptr);
    return result;
}

RESULT Level_Section_ReadSpriteSequences(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_sequences = File_ReadCountS32(file);
    LOG_DEBUG("sprite sequences: %d", num_sequences);

    int32_t injection_max_id = Inject_GetMaxStaticObject2DId();
    if (injection_max_id < 0) {
        injection_max_id = -1;
    }
    const int32_t capacity = MAX(num_sequences - 1, injection_max_id) + 1;
    Object_InitialiseStaticObjects2D(capacity);

    int32_t static_id = 0;
    for (int32_t i = 0; i < num_sequences; i++) {
        const int32_t id = File_ReadS32(file);
        const int16_t num_meshes = File_ReadS16(file);
        const int16_t mesh_idx = File_ReadS16(file);

        // In OG, a sprite was determined as either a game or static type based
        // on the original total game object count. As IDs are freely assignable
        // in TRX, a defined list of game sprites must instead be referred to.
        const OBJECT_ID object_id = Object_SlotToID(id);
        if (object_id != NO_OBJECT
            && Object_IsType(object_id, g_GameSpriteObjects)) {
            OBJECT *const obj = Object_Get(object_id);
            obj->mesh_count = num_meshes;
            obj->mesh_idx = mesh_idx;
            obj->anim_idx = NO_ANIM;
            obj->loaded = true;
        } else {
            STATIC_OBJECT_2D *const obj = Object_Get2DStatic(static_id);
            if (obj == nullptr) {
                result = FAIL("invalid sprite slot %d", id);
                goto finish;
            }
            obj->frame_count = ABS(num_meshes);
            obj->texture_idx = mesh_idx;
            obj->loaded = true;
            static_id++;
        }
    }

finish:
    Benchmark_End(&benchmark, nullptr);
    return result;
}

RESULT Level_Section_ReadItems(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;
    const int32_t num_items = File_ReadCountS32(file);
    LOG_INFO("items: %d", num_items);
    if (num_items > MAX_ITEMS) {
        result =
            FAIL("too many items: %d, at most %d fit", num_items, MAX_ITEMS);
        goto finish;
    }

    Item_InitialiseItems(num_items);
    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = Item_Get(i);
        const int16_t obj_id = File_ReadS16(file);
        item->object_id = Object_SlotToID(obj_id);
        if (item->object_id == NO_OBJECT) {
            result = FAIL("item %d names no object: %d", i, obj_id);
            goto finish;
        }

        item->room_num = File_ReadS16(file);
        M_ReadPosition(&item->pos, file);
        item->rot.y = File_ReadS16(file);
        M_ReadShade(loader, &item->shade, file);
        item->init_flags = File_ReadS16(file);
    }

finish:
    Benchmark_End(&benchmark, nullptr);
    return result;
}

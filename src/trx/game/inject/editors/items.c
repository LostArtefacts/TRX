#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/game/camera.h>
#include <trx/game/inject.h>
#include <trx/game/items.h>

static void M_ItemPosEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t item_num = File_ReadS16(injection->fp);
        const int16_t y_rot = File_ReadS16(injection->fp);
        const GAME_VECTOR pos = {
            .x = File_ReadS32(injection->fp),
            .y = File_ReadS32(injection->fp),
            .z = File_ReadS32(injection->fp),
            .room_num = File_ReadS16(injection->fp),
        };

        if (item_num < 0 || item_num >= Item_GetTotalCount()) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM *const item = Item_Get(item_num);
        item->rot.y = y_rot;
        item->pos = pos.pos;
        item->room_num = pos.room_num;
    }
}

static void M_ItemFlagEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t item_num = File_ReadS16(injection->fp);
        const INJECTION_OBJECT_INFO obj_info = Inject_ReadObjectPtr(injection);
        const uint16_t flags = File_ReadU16(injection->fp);

        if (item_num < 0 || item_num >= Item_GetTotalCount()) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        ITEM *const item = Item_Get(item_num);
        item->object_id = obj_info.id;
        item->init_flags = flags;
    }
}

static void M_ItemNameEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t item_num = File_ReadS16(injection->fp);
        const int32_t name_length = File_ReadS32(injection->fp);
        if (name_length <= 0) {
            continue;
        }

        if (name_length > 4096) {
            LOG_WARNING("Item name too long %d", name_length);
            File_Skip(injection->fp, name_length);
            continue;
        }

        if (item_num < 0 || item_num >= Item_GetTotalCount()) {
            LOG_WARNING("Item number %d is out of level item range", item_num);
            continue;
        }

        char *name = Memory_Alloc((size_t)(name_length + 1));
        File_ReadData(injection->fp, name, name_length);
        name[name_length] = '\0';

        if (!Item_SetName(item_num, name)) {
            LOG_WARNING(
                "Failed to set name on item %d: '%s' is already in use",
                item_num, name);
        }

        Memory_FreePointer(&name);
    }
}

static void M_CameraEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t camera_num = File_ReadS16(injection->fp);
        const XYZ_32 pos = {
            .x = File_ReadS32(injection->fp),
            .y = File_ReadS32(injection->fp),
            .z = File_ReadS32(injection->fp),
        };
        const int16_t room_num = File_ReadS16(injection->fp);
        const int16_t flags = File_ReadS16(injection->fp);
        if (ctx->mode == INJECTION_MODE_STATS) {
            continue;
        }

        if (camera_num < 0 || camera_num >= Camera_GetFixedObjectCount()) {
            LOG_WARNING(
                "Camera number %d is out of level camera range", camera_num);
            continue;
        }

        OBJECT_VECTOR *const camera = Camera_GetFixedObject(camera_num);
        camera->pos = pos;
        camera->data = room_num;
        camera->flags = flags;
    }
}

REGISTER_INJECT_EDITOR(IDT_ITEM_POS_EDITS, M_ItemPosEdits)
REGISTER_INJECT_EDITOR(IDT_ITEM_FLAG_EDITS, M_ItemFlagEdits)
REGISTER_INJECT_EDITOR(IDT_ITEM_NAME_EDITS, M_ItemNameEdits)
REGISTER_INJECT_EDITOR(IDT_CAMERA_EDITS, M_CameraEdits)

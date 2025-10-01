#include "game/camera.h"
#include "game/inject.h"
#include "game/items.h"
#include "log.h"

static void M_ItemEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t item_num = VFile_ReadS16(injection->fp);
        const int16_t y_rot = VFile_ReadS16(injection->fp);
        const GAME_VECTOR pos = {
            .x = VFile_ReadS32(injection->fp),
            .y = VFile_ReadS32(injection->fp),
            .z = VFile_ReadS32(injection->fp),
            .room_num = VFile_ReadS16(injection->fp),
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

static void M_CameraEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t camera_num = VFile_ReadS16(injection->fp);
        const XYZ_32 pos = {
            .x = VFile_ReadS32(injection->fp),
            .y = VFile_ReadS32(injection->fp),
            .z = VFile_ReadS32(injection->fp),
        };
        const int16_t room_num = VFile_ReadS16(injection->fp);
        const int16_t flags = VFile_ReadS16(injection->fp);

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

REGISTER_INJECT_EDITOR(IDT_ITEM_EDITS, M_ItemEdits)
REGISTER_INJECT_EDITOR(IDT_CAMERA_EDITS, M_CameraEdits)

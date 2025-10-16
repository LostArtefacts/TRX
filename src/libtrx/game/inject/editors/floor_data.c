#include "config.h"
#include "game/camera.h"
#include "game/inject.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "log.h"

#define NULL_FD_INDEX ((uint16_t)(-1))

static void M_TriggerTypeChange(
    const INJECTION *const injection, const SECTOR *const sector)
{
    const uint8_t new_type = VFile_ReadU8(injection->fp);
    if (sector != nullptr && sector->trigger != nullptr) {
        sector->trigger->type = new_type;
    }
}

static void M_TriggerParameterChange(
    const INJECTION *const injection, const SECTOR *const sector)
{
    const uint8_t cmd_type = VFile_ReadU8(injection->fp);
    const int16_t old_param = VFile_ReadS16(injection->fp);
    const int16_t new_param = VFile_ReadS16(injection->fp);
    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }

    // If we can find an action item for the given sector that matches
    // the command type and old (current) parameter, change it to the
    // new parameter.
    TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != cmd_type) {
            continue;
        }

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == old_param) {
                cam_data->camera_num = new_param;
                break;
            }
        } else {
            if ((int16_t)(intptr_t)cmd->parameter == old_param) {
                cmd->parameter = (void *)(intptr_t)new_param;
                break;
            }
        }
    }
}

static void M_SetMusicOneShot(const SECTOR *const sector)
{
    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CD) {
            sector->trigger->one_shot = true;
            break;
        }
    }
}

static void M_FixGlideCamera(
    const INJECTION *const injection, const SECTOR *const sector)
{
    const uint8_t camera_timer = VFile_ReadU8(injection->fp);
    const uint8_t glide_timer = VFile_ReadU8(injection->fp);
    const XYZ_16 camera_shift = {
        .x = VFile_ReadS16(injection->fp),
        .y = VFile_ReadS16(injection->fp),
        .z = VFile_ReadS16(injection->fp),
    };

    if (sector == nullptr || sector->trigger == nullptr) {
        return;
    }
#if TR_VERSION >= 2
    if (!g_Config.visuals.fix_glide_cameras) {
        return;
    }
#endif

    const TRIGGER_CMD *cmd = sector->trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type != TO_CAMERA) {
            continue;
        }

        TRIGGER_CAMERA_DATA *const cam_data =
            (TRIGGER_CAMERA_DATA *)cmd->parameter;
        cam_data->timer = camera_timer;
        cam_data->glide = glide_timer << 3;

        OBJECT_VECTOR *const camera =
            Camera_GetFixedObject(cam_data->camera_num);
        camera->pos.x += camera_shift.x;
        camera->pos.y += camera_shift.y;
        camera->pos.z += camera_shift.z;
        break;
    }
}

static void M_InsertFloorData(
    const INJECTION *const injection, SECTOR *const sector)
{
    const int32_t data_length = VFile_ReadS32(injection->fp);
    int16_t data[data_length];
    VFile_Read(injection->fp, data, sizeof(int16_t) * data_length);

    if (sector == nullptr) {
        return;
    }

    // This will reset all FD properties in the sector based on the raw data
    // imported. We pass a dummy null index to allow it to read from the
    // beginning of the array.
    Room_PopulateSectorData(sector, data, 0, NULL_FD_INDEX);
}

static void M_RoomShift(
    const INJECTION *const injection, const int16_t room_num)
{
    const uint32_t x_shift = ROUND_TO_SECTOR(VFile_ReadU32(injection->fp));
    const uint32_t z_shift = ROUND_TO_SECTOR(VFile_ReadU32(injection->fp));
    const int32_t y_shift = ROUND_TO_CLICK(VFile_ReadS32(injection->fp));

    ROOM *const room = Room_Get(room_num);
    room->pos.x += x_shift;
    room->pos.z += z_shift;
    room->min_floor += y_shift;
    room->max_ceiling += y_shift;

    // Move any items in the room to match.
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->room_num != room_num) {
            continue;
        }

        item->pos.x += x_shift;
        item->pos.y += y_shift;
        item->pos.z += z_shift;
    }

    if (y_shift == 0) {
        return;
    }

    // Update the sector floor and ceiling heights to match.
    for (int32_t i = 0; i < room->size.z * room->size.x; i++) {
        SECTOR *const sector = &room->sectors[i];
        if (sector->floor.height == NO_HEIGHT
            || sector->ceiling.height == NO_HEIGHT) {
            continue;
        }

        sector->floor.height += y_shift;
        sector->ceiling.height += y_shift;
    }

    // Update vertex Y values to match; x and z are room-relative.
    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        room->mesh.vertices[i].pos.y += y_shift;
    }
}

static void M_TriggeredItem(const INJECTION *const injection)
{
    if (Item_GetLevelCount() == MAX_ITEMS) {
        VFile_Skip(
            injection->fp,
            sizeof(int16_t) * 4 + sizeof(int32_t) * 3 + sizeof(uint16_t));
        LOG_WARNING("Cannot add more than %d items", MAX_ITEMS);
        return;
    }

    const int16_t item_num = Item_CreateLevelItem();
    ITEM *const item = Item_Get(item_num);

    const INJECTION_OBJECT_INFO obj_info = Inject_ReadObjectPtr(injection);
    item->object_id = obj_info.id;
    item->room_num = VFile_ReadS16(injection->fp);
    item->pos.x = VFile_ReadS32(injection->fp);
    item->pos.y = VFile_ReadS32(injection->fp);
    item->pos.z = VFile_ReadS32(injection->fp);
    item->rot.y = VFile_ReadS16(injection->fp);
    item->shade.value_1 = VFile_ReadS16(injection->fp);
    if (g_TRVersion >= 2) {
        item->shade.value_2 = item->shade.value_1;
    }
    item->flags = VFile_ReadU16(injection->fp);
}

static void M_RoomProperties(
    const INJECTION *const injection, const int16_t room_num)
{
    const uint16_t flags = VFile_ReadU16(injection->fp);
    ROOM *const room = Room_Get(room_num);
    room->flags = flags;
}

static void M_SectorOverwrite(
    const INJECTION *const injection, SECTOR *const sector)
{
    const uint16_t fd_idx = VFile_ReadU16(injection->fp);
    const int16_t box_idx = VFile_ReadS16(injection->fp);
    const int16_t pit_room = VFile_ReadS16(injection->fp);
    const int16_t floor = VFile_ReadS16(injection->fp);
    const int16_t sky_room = VFile_ReadS16(injection->fp);
    const int16_t ceiling = VFile_ReadS16(injection->fp);

    if (sector == nullptr) {
        return;
    }

    sector->idx = fd_idx;
    sector->box = box_idx;
    sector->portal_room.pit = pit_room;
    sector->floor.height = floor;
    sector->portal_room.sky = sky_room;
    sector->ceiling.height = ceiling;
}

static void M_FixZones(
    const INJECTION *const injection, const SECTOR *const sector)
{
    if (sector == nullptr || sector->box == NO_BOX) {
        VFile_Skip(
            injection->fp, 2 * sizeof(int16_t) * (Box_GetZoneCount() + 1));
        return;
    }

    const int16_t box_idx = sector->box;
    for (int32_t flip_status = 0; flip_status < 2; flip_status++) {
        for (int32_t zone_idx = 0; zone_idx < Box_GetZoneCount(); zone_idx++) {
            int16_t *const ground_zone =
                Box_GetGroundZone(flip_status, zone_idx);
            ground_zone[box_idx] = VFile_ReadS16(injection->fp);
        }

        int16_t *const fly_zone = Box_GetFlyZone(flip_status);
        fly_zone[box_idx] = VFile_ReadS16(injection->fp);
    }
}

static void M_SetSectorPortals(
    const INJECTION *const injection, SECTOR *const sector)
{
    if (sector == nullptr) {
        VFile_Skip(injection->fp, 3 * sizeof(int16_t));
        return;
    }

    sector->portal_room.wall = VFile_ReadS16(injection->fp);
    sector->portal_room.sky = VFile_ReadS16(injection->fp);
    sector->portal_room.pit = VFile_ReadS16(injection->fp);
}

static void M_SetSectorClimbability(
    const INJECTION *const injection, SECTOR *const sector)
{
    const int32_t direction = VFile_ReadS32(injection->fp);
    if (sector != nullptr) {
        sector->ladder = (LADDER_DIRECTION)(direction & 0xF);
    }
}

static void M_FloorDataEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const int16_t room_num = VFile_ReadS16(injection->fp);
        const uint16_t x = VFile_ReadU16(injection->fp);
        const uint16_t z = VFile_ReadU16(injection->fp);
        const int32_t fd_edit_count = VFile_ReadS32(injection->fp);

        // Verify that the given room and coordinates are accurate.
        // Individual FD functions must check that sector is actually set.
        const ROOM *room = nullptr;
        SECTOR *sector = nullptr;
        if (room_num < 0 || room_num >= Room_GetCount()) {
            LOG_WARNING("Room index %d is invalid", room_num);
        } else {
            room = Room_Get(room_num);
            if (x >= room->size.x || z >= room->size.z) {
                LOG_WARNING(
                    "Sector [%d,%d] is invalid for room %d", x, z, room_num);
            } else {
                sector = Room_GetUnitSector(room, x, z);
            }
        }

        for (int32_t j = 0; j < fd_edit_count; j++) {
            const FLOOR_EDIT_TYPE edit_type = VFile_ReadS32(injection->fp);
            switch (edit_type) {
            case FET_TRIGGER_TYPE:
                M_TriggerTypeChange(injection, sector);
                break;
            case FET_TRIGGER_PARAM:
                M_TriggerParameterChange(injection, sector);
                break;
            case FET_MUSIC_ONESHOT:
                M_SetMusicOneShot(sector);
                break;
            case FET_GLIDE_CAMERA:
                M_FixGlideCamera(injection, sector);
                break;
            case FET_FD_INSERT:
                M_InsertFloorData(injection, sector);
                break;
            case FET_ROOM_SHIFT:
                M_RoomShift(injection, room_num);
                break;
            case FET_TRIGGER_ITEM:
                M_TriggeredItem(injection);
                break;
            case FET_ROOM_PROPERTIES:
                M_RoomProperties(injection, room_num);
                break;
            case FET_SECTOR_OVERWRITE:
                M_SectorOverwrite(injection, sector);
                break;
            case FET_ZONE_FIX:
                M_FixZones(injection, sector);
                break;
            case FET_PORTALS:
                M_SetSectorPortals(injection, sector);
                break;
            case FET_CLIMB:
                M_SetSectorClimbability(injection, sector);
                break;
            default:
                LOG_WARNING("Unknown floor data edit type: %d", edit_type);
                break;
            }
        }
    }
}

REGISTER_INJECT_EDITOR(IDT_FLOOR_EDITS, M_FloorDataEdits)

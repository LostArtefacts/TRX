#include "game/rooms/floor_data.h"

#include "game/game_buf.h"
#include "game/items.h"
#include "game/rooms.h"

#define M_NULL_INDEX 0
#define M_IS_DONE(t) ((t & 0x8000) == 0x8000)

#define M_ENTRY_TYPE(t) (t & 0x1F)
#define M_TRIG_TYPE(t) ((t & 0x7F00) >> 8)
#define M_TRIG_TIMER(t) (t & 0xFF)
#define M_TRIG_ONE_SHOT(t) ((t & 0x100) == 0x100)
#define M_TRIG_MASK(t) (t & 0x3E00)
#define M_TRIG_CMD_TYPE(t) ((t & 0x7C00) >> 10)
#define M_TRIG_CMD_ARG(t) (t & 0x3FF)
#define M_TRIG_CAM_GLIDE(t) ((t & 0x3E00) >> 6)

#if TR_VERSION == 2
    #define M_LADDER_TYPE(t) ((t & 0x7F00) >> 8)
#endif

static const int16_t *M_ReadTrigger(
    const int16_t *data, int16_t fd_entry, SECTOR *sector);

static const int16_t *M_ReadTrigger(
    const int16_t *data, const int16_t fd_entry, SECTOR *const sector)
{
    TRIGGER *const trigger = GameBuf_Alloc(sizeof(TRIGGER), GBUF_FLOOR_DATA);

    const int16_t trig_setup = *data++;
    trigger->type = M_TRIG_TYPE(fd_entry);
    trigger->timer = M_TRIG_TIMER(trig_setup);
    trigger->one_shot = M_TRIG_ONE_SHOT(trig_setup);
    trigger->mask = M_TRIG_MASK(trig_setup);
    trigger->item_index = NO_ITEM;

    if (trigger->type == TT_SWITCH || trigger->type == TT_KEY
        || trigger->type == TT_PICKUP) {
        const int16_t item_data = *data++;
        trigger->item_index = M_TRIG_CMD_ARG(item_data);
        if (M_IS_DONE(item_data)) {
            return data;
        }
    }

    TRIGGER_CMD *cmd;
    if (sector->trigger == nullptr) {
        sector->trigger = trigger;
        sector->trigger->command =
            GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = sector->trigger->command;
    } else {
        // Some old TRLEs have incorrectly formatted floor data, with multiple
        // trigger entries defined where regular triggers overlap dummies. In
        // this case we link the new commands onto the old.
        cmd = sector->trigger->command;
        while (cmd->next_cmd != nullptr) {
            cmd = cmd->next_cmd;
        }
        cmd->next_cmd = GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = cmd->next_cmd;
    }

    while (true) {
        int16_t command = *data++;
        cmd->type = M_TRIG_CMD_TYPE(command);

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                GameBuf_Alloc(sizeof(TRIGGER_CAMERA_DATA), GBUF_FLOOR_DATA);
            cmd->parameter = (void *)cam_data;
            cam_data->camera_num = M_TRIG_CMD_ARG(command);

            command = *data++;
            cam_data->timer = M_TRIG_TIMER(command);
            cam_data->glide = M_TRIG_CAM_GLIDE(command);
            cam_data->one_shot = M_TRIG_ONE_SHOT(command);
        } else {
            cmd->parameter = (void *)(intptr_t)M_TRIG_CMD_ARG(command);
        }

        if (M_IS_DONE(command)) {
            cmd->next_cmd = nullptr;
            break;
        }

        cmd->next_cmd = GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = cmd->next_cmd;
    }

    return data;
}

void Room_ParseFloorData(const int16_t *floor_data)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->size.x * room->size.z; j++) {
            SECTOR *const sector = &room->sectors[j];
            Room_PopulateSectorData(
                sector, floor_data, sector->idx, M_NULL_INDEX);
        }
    }
}

void Room_PopulateSectorData(
    SECTOR *const sector, const int16_t *floor_data, const uint16_t start_index,
    const uint16_t null_index)
{
    sector->floor.tilt = 0;
    sector->ceiling.tilt = 0;
    sector->portal_room.wall = NO_ROOM;
    sector->is_death_sector = false;
    sector->trigger = nullptr;
#if TR_VERSION == 2
    sector->ladder = LADDER_NONE;
#endif

    if (start_index == null_index) {
        return;
    }

    const int16_t *data = &floor_data[start_index];
    int16_t fd_entry;
    do {
        fd_entry = *data++;

        switch (M_ENTRY_TYPE(fd_entry)) {
        case FT_TILT:
            sector->floor.tilt = *data++;
            break;

        case FT_ROOF:
            sector->ceiling.tilt = *data++;
            break;

        case FT_DOOR:
            sector->portal_room.wall = *data++;
            break;

        case FT_LAVA:
            sector->is_death_sector = true;
            break;

        case FT_TRIGGER:
            data = M_ReadTrigger(data, fd_entry, sector);
            break;

#if TR_VERSION >= 2
        case FT_CLIMB:
            sector->ladder = (LADDER_DIRECTION)M_LADDER_TYPE(fd_entry);
            break;
#endif

        default:
            break;
        }
    } while (!M_IS_DONE(fd_entry));
}

void Room_TestTriggers(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector =
        Room_GetSector(item->pos.x, MAX_HEIGHT, item->pos.z, &room_num);

    Room_TestSectorTrigger(item, sector);
#if TR_VERSION == 1
    if (item->object_id != O_TORSO) {
        return;
    }

    for (int32_t dx = -1; dx < 2; dx++) {
        for (int32_t dz = -1; dz < 2; dz++) {
            if (dx == 0 && dz == 0) {
                continue;
            }

            room_num = item->room_num;
            sector = Room_GetSector(
                item->pos.x + dx * WALL_L, MAX_HEIGHT,
                item->pos.z + dz * WALL_L, &room_num);
            Room_TestSectorTrigger(item, sector);
        }
    }
#endif
}

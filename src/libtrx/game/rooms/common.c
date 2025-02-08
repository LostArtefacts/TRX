#include "game/rooms/common.h"

#include "debug.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/items.h"
#include "game/rooms/const.h"
#include "game/rooms/enum.h"
#include "utils.h"

#define FD_NULL_INDEX 0
#define FD_IS_DONE(t) ((t & 0x8000) == 0x8000)

#define FD_ENTRY_TYPE(t) (t & 0x1F)
#define FD_TRIG_TYPE(t) ((t & 0x7F00) >> 8)
#define FD_TRIG_TIMER(t) (t & 0xFF)
#define FD_TRIG_ONE_SHOT(t) ((t & 0x100) == 0x100)
#define FD_TRIG_MASK(t) (t & 0x3E00)
#define FD_TRIG_CMD_TYPE(t) ((t & 0x7C00) >> 10)
#define FD_TRIG_CMD_ARG(t) (t & 0x3FF)
#define FD_TRIG_CAM_GLIDE(t) ((t & 0x3E00) >> 6)

#if TR_VERSION == 2
    #define FD_LADDER_TYPE(t) ((t & 0x7F00) >> 8)
#endif

static int32_t m_RoomCount = 0;
static ROOM *m_Rooms = nullptr;
static bool m_FlipStatus = false;
static int32_t m_FlipEffect = -1;
static int32_t m_FlipTimer = 0;

static const int16_t *M_ReadTrigger(
    const int16_t *data, int16_t fd_entry, SECTOR *sector);

static const int16_t *M_ReadTrigger(
    const int16_t *data, const int16_t fd_entry, SECTOR *const sector)
{
    TRIGGER *const trigger = GameBuf_Alloc(sizeof(TRIGGER), GBUF_FLOOR_DATA);

    const int16_t trig_setup = *data++;
    trigger->type = FD_TRIG_TYPE(fd_entry);
    trigger->timer = FD_TRIG_TIMER(trig_setup);
    trigger->one_shot = FD_TRIG_ONE_SHOT(trig_setup);
    trigger->mask = FD_TRIG_MASK(trig_setup);
    trigger->item_index = NO_ITEM;

    if (trigger->type == TT_SWITCH || trigger->type == TT_KEY
        || trigger->type == TT_PICKUP) {
        const int16_t item_data = *data++;
        trigger->item_index = FD_TRIG_CMD_ARG(item_data);
        if (FD_IS_DONE(item_data)) {
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
        cmd->type = FD_TRIG_CMD_TYPE(command);

        if (cmd->type == TO_CAMERA) {
            TRIGGER_CAMERA_DATA *const cam_data =
                GameBuf_Alloc(sizeof(TRIGGER_CAMERA_DATA), GBUF_FLOOR_DATA);
            cmd->parameter = (void *)cam_data;
            cam_data->camera_num = FD_TRIG_CMD_ARG(command);

            command = *data++;
            cam_data->timer = FD_TRIG_TIMER(command);
            cam_data->glide = FD_TRIG_CAM_GLIDE(command);
            cam_data->one_shot = FD_TRIG_ONE_SHOT(command);
        } else {
            cmd->parameter = (void *)(intptr_t)FD_TRIG_CMD_ARG(command);
        }

        if (FD_IS_DONE(command)) {
            cmd->next_cmd = nullptr;
            break;
        }

        cmd->next_cmd = GameBuf_Alloc(sizeof(TRIGGER_CMD), GBUF_FLOOR_DATA);
        cmd = cmd->next_cmd;
    }

    return data;
}

void Room_InitialiseRooms(const int32_t num_rooms)
{
    m_RoomCount = num_rooms;
    m_Rooms = num_rooms == 0
        ? nullptr
        : GameBuf_Alloc(sizeof(ROOM) * num_rooms, GBUF_ROOMS);
}

int32_t Room_GetCount(void)
{
    return m_RoomCount;
}

ROOM *Room_Get(const int32_t room_num)
{
    if (m_Rooms == nullptr) {
        return nullptr;
    }
    return &m_Rooms[room_num];
}

void Room_InitialiseFlipStatus(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room == -1) {
            room->flip_status = RFS_NONE;
        } else if (room->flip_status != RFS_FLIPPED) {
            ROOM *const flipped_room = Room_Get(room->flipped_room);
            room->flip_status = RFS_UNFLIPPED;
            flipped_room->flip_status = RFS_FLIPPED;
        }
    }

    m_FlipStatus = false;
    m_FlipEffect = -1;
    m_FlipTimer = 0;
}

bool Room_GetFlipStatus(void)
{
    return m_FlipStatus;
}

void Room_ToggleFlipStatus(void)
{
    m_FlipStatus = !m_FlipStatus;
}

int32_t Room_GetFlipEffect(void)
{
    return m_FlipEffect;
}

void Room_SetFlipEffect(const int32_t flip_effect)
{
    m_FlipEffect = flip_effect;
}

int32_t Room_GetFlipTimer(void)
{
    return m_FlipTimer;
}

void Room_SetFlipTimer(const int32_t flip_timer)
{
    m_FlipTimer = flip_timer;
}

void Room_ParseFloorData(const int16_t *floor_data)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        for (int32_t j = 0; j < room->size.x * room->size.z; j++) {
            SECTOR *const sector = &room->sectors[j];
            Room_PopulateSectorData(
                &room->sectors[j], floor_data, sector->idx, FD_NULL_INDEX);
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

        switch (FD_ENTRY_TYPE(fd_entry)) {
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
            sector->ladder = (LADDER_DIRECTION)FD_LADDER_TYPE(fd_entry);
            break;
#endif

        default:
            break;
        }
    } while (!FD_IS_DONE(fd_entry));
}

int32_t Room_GetAdjoiningRooms(
    int16_t init_room_num, int16_t out_room_nums[],
    const int32_t max_room_num_count)
{
    int32_t count = 0;
    if (max_room_num_count >= 1) {
        out_room_nums[count++] = init_room_num;
    }

    const PORTALS *const portals = Room_Get(init_room_num)->portals;
    if (portals != nullptr) {
        for (int32_t i = 0; i < portals->count; i++) {
            if (count >= max_room_num_count) {
                break;
            }
            const int16_t room_num = portals->portal[i].room_num;
            out_room_nums[count++] = room_num;
        }
    }

    return count;
}

int16_t Room_GetIndexFromPos(const int32_t x, const int32_t y, const int32_t z)
{
    // TODO: merge this to Room_FindByPos!
    const int32_t room_num = Room_FindByPos(x, y, z);
    if (room_num == NO_ROOM_NEG) {
        return NO_ROOM;
    }
    return room_num;
}

int32_t Room_FindByPos(const int32_t x, const int32_t y, const int32_t z)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        if (room->flip_status == RFS_FLIPPED) {
            continue;
        }
        const int32_t x1 = room->pos.x + WALL_L;
        const int32_t x2 = room->pos.x + (room->size.x - 1) * WALL_L;
        const int32_t y1 = room->max_ceiling;
        const int32_t y2 = room->min_floor;
        const int32_t z1 = room->pos.z + WALL_L;
        const int32_t z2 = room->pos.z + (room->size.z - 1) * WALL_L;
        if (x >= x1 && x < x2 && y >= y1 && y <= y2 && z >= z1 && z < z2) {
            return i;
        }
    }

    return NO_ROOM_NEG;
}

BOUNDS_32 Room_GetWorldBounds(void)
{
    BOUNDS_32 bounds = {
        .min.x = 0x7FFFFFFF,
        .min.z = 0x7FFFFFFF,
        .max.x = 0,
        .max.z = 0,
        .min.y = MAX_HEIGHT,
        .max.y = -MAX_HEIGHT,
    };

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        bounds.min.x = MIN(bounds.min.x, room->pos.x);
        bounds.max.x = MAX(bounds.max.x, room->pos.x + room->size.x * WALL_L);
        bounds.min.z = MIN(bounds.min.z, room->pos.z);
        bounds.max.z = MAX(bounds.max.z, room->pos.z + room->size.z * WALL_L);
        bounds.min.y = MIN(bounds.min.y, room->max_ceiling);
        bounds.max.y = MAX(bounds.max.y, room->min_floor);
    }

    return bounds;
}

SECTOR *Room_GetWorldSector(
    const ROOM *const room, const int32_t x_pos, const int32_t z_pos)
{
    const int32_t x_sector = (x_pos - room->pos.x) >> WALL_SHIFT;
    const int32_t z_sector = (z_pos - room->pos.z) >> WALL_SHIFT;
    return Room_GetUnitSector(room, x_sector, z_sector);
}

SECTOR *Room_GetUnitSector(
    const ROOM *const room, const int32_t x_sector, const int32_t z_sector)
{
    return &room->sectors[z_sector + x_sector * room->size.z];
}

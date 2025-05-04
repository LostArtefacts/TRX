#include "game/rooms/common.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/objects/common.h"
#include "game/objects/traps/movable_block.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/sound/common.h"
#include "utils.h"

static int32_t m_RoomCount = 0;
static ROOM *m_Rooms = nullptr;
static bool m_FlipStatus = false;
static int32_t m_FlipEffect = -1;
static int32_t m_FlipTimer = 0;
static int32_t m_FlipSlotFlags[MAX_FLIP_MAPS] = {};

static void M_AddFlipItems(const ROOM *room);
static void M_RemoveFlipItems(const ROOM *room);

static void M_AddFlipItems(const ROOM *const room)
{
    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->handle_flip_func != nullptr) {
            obj->handle_flip_func(item, RFS_FLIPPED);
        }

        item_num = item->next_item;
    }
}

static void M_RemoveFlipItems(const ROOM *const room)
{
    int16_t item_num = room->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);

        if (obj->handle_flip_func != nullptr) {
            obj->handle_flip_func(item, RFS_UNFLIPPED);
        }

        // TR2 does not have land/water objects like crocodile/alligator in TR1,
        // so avoid instances of floating water creatures in drained rooms.
        if (TR_VERSION == 2 && (item->flags & IF_ONE_SHOT) && obj->intelligent
            && item->hit_points <= 0) {
            Item_RemoveDrawn(item_num);
            item->flags |= IF_KILLED;
        }

        item_num = item->next_item;
    }
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

int32_t Room_GetNumber(const ROOM *const room)
{
    if (room == nullptr) {
        return NO_ROOM_NEG;
    }
    return room - m_Rooms;
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
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        m_FlipSlotFlags[i] = 0;
    }
}

void Room_FlipMap(void)
{
    Sound_StopAmbientSounds();
    MovableBlock_HandleFlipMap(RFS_UNFLIPPED);

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room < 0) {
            continue;
        }

        M_RemoveFlipItems(room);

        ROOM *const flipped = Room_Get(room->flipped_room);
        const ROOM temp = *room;
        *room = *flipped;
        *flipped = temp;

        room->flipped_room = flipped->flipped_room;
        flipped->flipped_room = -1;
        room->flip_status = RFS_UNFLIPPED;
        flipped->flip_status = RFS_FLIPPED;

        room->item_num = flipped->item_num;
        room->effect_num = flipped->effect_num;

        M_AddFlipItems(room);
        Output_ObserveRoomFlip(flipped);
        Output_ObserveRoomFlip(room);
    }

    MovableBlock_HandleFlipMap(RFS_FLIPPED);
    m_FlipStatus = !m_FlipStatus;
}

bool Room_GetFlipStatus(void)
{
    return m_FlipStatus;
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

void Room_IncrementFlipTimer(const int32_t num_frames)
{
    m_FlipTimer += num_frames;
}

int32_t Room_GetFlipSlotFlags(const int32_t slot_idx)
{
    return m_FlipSlotFlags[slot_idx];
}

void Room_SetFlipSlotFlags(const int32_t slot_idx, const int32_t flags)
{
    m_FlipSlotFlags[slot_idx] = flags;
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

int32_t Room_GetFlippedBaseRoom(const int32_t room_num)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        if (room->flipped_room == room_num) {
            return i;
        }
    }
    return NO_ROOM;
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

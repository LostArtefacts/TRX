#include "game/rooms/common.h"

#include "debug.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/level.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/sound/common.h"
#include "utils.h"
#include "vector.h"

static int32_t m_RoomCount = 0;
static ROOM *m_Rooms = nullptr;
static bool m_FlipStatus = false;
static int32_t m_FlipEffect = -1;
static int32_t m_FlipTimer = 0;
static int32_t m_FlipSlotFlags[MAX_FLIP_MAPS] = {};

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

static void M_GetNewRoom(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    Room_GetSector(x, y, z, &room_num);
    Room_MarkToBeDrawn(room_num);
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
        return NO_ROOM;
    }
    return room - m_Rooms;
}

void Room_InitialiseFlipStatus(void)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        if (room->flipped_room == NO_ROOM) {
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
    Walkable_Reset();

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
        room->flip_status = RFS_UNFLIPPED;
        flipped->flipped_room = NO_ROOM;
        flipped->flip_status = RFS_FLIPPED;

        room->item_num = flipped->item_num;
        room->effect_num = flipped->effect_num;

        M_AddFlipItems(room);
    }

    m_FlipStatus = !m_FlipStatus;

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        if (room->flip_status != RFS_NONE) {
            Output_DispatchRoomFlip(room);
        }
    }

    Level_LoadWalkables();
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

    return NO_ROOM;
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
    BOUNDS_32 world_bounds = {
        .min.x = INT32_MAX,
        .min.z = INT32_MAX,
        .max.x = 0,
        .max.z = 0,
        .min.y = MAX_HEIGHT,
        .max.y = -MAX_HEIGHT,
    };
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const BOUNDS_32 room_bounds = Room_GetRoomBounds(i);
        world_bounds.min.x = MIN(world_bounds.min.x, room_bounds.min.x);
        world_bounds.max.x = MAX(world_bounds.max.x, room_bounds.max.x);
        world_bounds.min.z = MIN(world_bounds.min.z, room_bounds.min.z);
        world_bounds.max.z = MAX(world_bounds.max.z, room_bounds.max.z);
        world_bounds.min.y = MIN(world_bounds.min.y, room_bounds.min.y);
        world_bounds.max.y = MAX(world_bounds.max.y, room_bounds.max.y);
    }
    return world_bounds;
}

void Room_GetNearbyRooms(
    const int32_t x, const int32_t y, const int32_t z, const int32_t r,
    const int32_t h, const int16_t room_num)
{
    Room_DrawReset();
    Room_MarkToBeDrawn(room_num);

    M_GetNewRoom(r + x, y, r + z, room_num);
    M_GetNewRoom(x - r, y, r + z, room_num);
    M_GetNewRoom(r + x, y, z - r, room_num);
    M_GetNewRoom(x - r, y, z - r, room_num);
    M_GetNewRoom(r + x, y - h, r + z, room_num);
    M_GetNewRoom(x - r, y - h, r + z, room_num);
    M_GetNewRoom(r + x, y - h, z - r, room_num);
    M_GetNewRoom(x - r, y - h, z - r, room_num);
}

bool Room_CheckOverlap(const int16_t room_num_0, const int16_t room_num_1)
{
    const BOUNDS_32 room_0_bounds = Room_GetRoomBounds(room_num_0);
    const BOUNDS_32 room_1_bounds = Room_GetRoomBounds(room_num_1);

    // clang-format off
    return (
        room_0_bounds.min.x <= room_1_bounds.max.x &&
        room_0_bounds.max.x >= room_1_bounds.min.x &&
        room_0_bounds.min.y <= room_1_bounds.max.y &&
        room_0_bounds.max.y >= room_1_bounds.min.y &&
        room_0_bounds.min.z <= room_1_bounds.max.z &&
        room_0_bounds.max.z >= room_1_bounds.min.z);
    // clang-format on
}

bool Room_FindValidPos(XYZ_32 *const out_pos, int16_t *const out_room_num)
{
    ASSERT(out_pos != nullptr);
    ASSERT(out_room_num != nullptr);
    XYZ_32 initial_pos = *out_pos;
    int32_t x = out_pos->x;
    int32_t y = out_pos->y;
    int32_t z = out_pos->z;
    int16_t room_num = *out_room_num;
    if (room_num == NO_ROOM) {
        room_num = Room_GetIndexFromPos(x, y, z);
    }
    if (room_num == NO_ROOM) {
        return false;
    }

    const ROOM *const room = Room_Get(room_num);
    if (room->flip_status == RFS_FLIPPED && Room_GetFlipStatus()) {
        room_num = Room_GetFlippedBaseRoom(room_num);
        if (room_num == NO_ROOM) {
            return false;
        }
    }

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    int16_t height = Room_GetHeight(sector, x, y, z);

    if (height == NO_HEIGHT) {
        // Sample a sphere of points around target x, y, z
        // and teleport to the first available location.
        VECTOR *const points = Vector_Create(sizeof(XYZ_32));

        const int32_t radius = 10;
        const int32_t unit = STEP_L;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                if (SQUARE(dx) + SQUARE(dz) > SQUARE(radius)) {
                    continue;
                }

                const XYZ_32 point = {
                    .x = ROUND_TO_SECTOR(x + dx * unit) + WALL_L / 2,
                    .y = y,
                    .z = ROUND_TO_SECTOR(z + dz * unit) + WALL_L / 2,
                };
                sector = Room_GetSector(point.x, point.y, point.z, &room_num);
                height = Room_GetHeightEx(
                    sector, point.x, point.y, point.z, true, NO_ITEM);
                if (height == NO_HEIGHT) {
                    continue;
                }
                Vector_Add(points, (void *)&point);
            }
        }

        int32_t best_distance = INT32_MAX;
        for (int32_t i = 0; i < points->count; i++) {
            const XYZ_32 *const point = (const XYZ_32 *)Vector_Get(points, i);
            const int32_t distance = XYZ_32_GetDistance(point, &initial_pos);
            if (distance < best_distance) {
                best_distance = distance;
                x = point->x;
                y = point->y;
                z = point->z;
            }
        }

        Vector_Free(points);
        if (best_distance == INT32_MAX) {
            return false;
        }
    }

    out_pos->x = x;
    out_pos->y = y;
    out_pos->z = z;
    *out_room_num = room_num;

    return true;
}

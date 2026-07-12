// A room table with no engine behind it, so the tests can drive the real room
// surface with no level loaded.

#include "fake_engine_rooms.h"

#include <trx/game/items/actions/ids.h>
#include <trx/game/rooms.h>

static ROOM m_Rooms[FAKE_ROOM_COUNT];

FAKE_ROOM_CALLS g_FakeRoomCalls;

void FakeRooms_Reset(void)
{
    g_FakeRoomCalls = (FAKE_ROOM_CALLS) { 0 };

    for (int32_t i = 0; i < FAKE_ROOM_COUNT; i++) {
        m_Rooms[i] = (ROOM) {
            .pos = { .x = i * 1024, .y = 0, .z = 0 },
            .size = { .x = 4, .z = 4 },
            .min_floor = 0,
            .max_ceiling = -2048,
            .flipped_room = -1,
            .flip_status = RFS_NONE,
        };
    }

    // Rooms 1 and 2 are a flip pair.
    m_Rooms[0].flipped_room = 1;
    m_Rooms[0].flip_status = RFS_UNFLIPPED;
    m_Rooms[1].flip_status = RFS_FLIPPED;
}

int32_t Room_GetCount(void)
{
    return FAKE_ROOM_COUNT;
}

ROOM *Room_Get(const int32_t room_num)
{
    return &m_Rooms[room_num];
}

BOUNDS_32 Room_GetRoomBounds(const ROOM *const room)
{
    return (BOUNDS_32) {
        .min = { .x = room->pos.x, .y = room->max_ceiling, .z = room->pos.z },
        .max = {
            .x = room->pos.x + room->size.x * 1024,
            .y = room->min_floor,
            .z = room->pos.z + room->size.z * 1024,
        },
    };
}

void Room_FlipMap(void)
{
    g_FakeRoomCalls.flip_map++;
}

void Room_SetFlipEffect(const int32_t flip_effect)
{
    g_FakeRoomCalls.flip_effect = flip_effect;
}

void Room_SetFlipTimer(const int32_t flip_timer)
{
    g_FakeRoomCalls.flip_timer = flip_timer;
}

bool Room_FindValidPos(XYZ_32 *const out_pos, int16_t *const out_room_num)
{
    if (out_pos->x < 0) {
        return false;
    }
    out_pos->y += 16;
    *out_room_num = 0;
    return true;
}

ITEM_ACTION ItemAction_ToGameID(const ITEM_TRX_ACTION action)
{
    return (ITEM_ACTION)action;
}

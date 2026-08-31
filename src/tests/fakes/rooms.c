// A room table with no engine behind it, so the tests can drive the real room
// surface with no level loaded.

#include <fakes/rooms.h>

#include <harness/fake_calls.h>

#include <trx/core/handle.h>
#include <trx/game/items/actions.h>
#include <trx/game/rooms.h>

static ROOM m_Rooms[FAKE_ROOM_COUNT];
static HANDLE_EPOCH m_RoomEpoch;
static bool m_FlipStatus;
static ITEM_ACTION_INTERCEPTOR m_Interceptor;

static void M_Reset(void)
{
    Handle_EpochBump(&m_RoomEpoch);
    m_FlipStatus = false;

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

// What loading the next level does to the room table: the rooms are replaced,
// so every handle to one of the old ones is stale.
void FakeRooms_LoadNextLevel(void)
{
    Handle_EpochBump(&m_RoomEpoch);
}

FAKE_ON_RESET(M_Reset)

int32_t Room_GetCount(void)
{
    return FAKE_ROOM_COUNT;
}

TRX_HANDLE Room_GetHandle(const int32_t room_num)
{
    return Handle_EpochMint(&m_RoomEpoch, room_num);
}

ROOM *Room_FromHandle(const TRX_HANDLE handle)
{
    if (!Handle_EpochIsLive(&m_RoomEpoch, handle)) {
        return nullptr;
    }
    return Room_Get(handle.id);
}

ROOM *Room_Get(const int32_t room_num)
{
    if (room_num < 0 || room_num >= FAKE_ROOM_COUNT) {
        return nullptr;
    }
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

// As the engine's own test: the room's bounds, and then a floor in the column
// the point stands in. The fake rooms are a sector apart and four sectors wide,
// so a point sits inside several of them at once.
bool Room_PointInside(const ROOM *const room, const XYZ_32 point)
{
    if (room == nullptr) {
        return false;
    }
    const BOUNDS_32 bounds = Room_GetRoomBounds(room);
    if (point.x < bounds.min.x || point.x >= bounds.max.x
        || point.y < bounds.min.y || point.y > bounds.max.y
        || point.z < bounds.min.z || point.z >= bounds.max.z) {
        return false;
    }
    // The fake level's floor rule, as Room_GetHeightEx has it.
    return point.x >= 0;
}

void Room_FlipMap(const int32_t group)
{
    FAKE_RECORD("flip_map", FV(group));
    m_FlipStatus = !m_FlipStatus;
}

void Room_SetFlipGroup(const int32_t room_num, const int32_t group)
{
    FAKE_RECORD("set_flip_group", FV(room_num), FV(group));
}

bool Room_GetFlipStatus(void)
{
    return m_FlipStatus;
}

bool Room_GetFlipGroupStatus(const int32_t group)
{
    return m_FlipStatus;
}

void Room_SetFlipEffect(const int32_t flip_effect)
{
    FAKE_RECORD("set_flip_effect", FV(flip_effect));
}

void ItemAction_SetInterceptor(const ITEM_ACTION_INTERCEPTOR interceptor)
{
    m_Interceptor = interceptor;
}

bool ItemAction_Intercept(
    const int32_t effect_num, const int32_t timer, const int16_t item_num)
{
    return m_Interceptor != nullptr
        && m_Interceptor(effect_num, timer, item_num);
}

void Room_SetFlipTimer(const int32_t flip_timer)
{
    FAKE_RECORD("set_flip_timer", FV(flip_timer));
}

// The fake level's floor is flat at y = 0, and there is none west of the
// origin: that is how a test says a position has nothing to stand on.
SECTOR *Room_GetSector(const XYZ_32 pos, int16_t *const room_num)
{
    static SECTOR sector;
    return &sector;
}

int32_t Room_GetHeightEx(
    const SECTOR *const sector, const XYZ_32 pos, const bool fix_tilts,
    const int16_t ignore_item_num)
{
    FAKE_RECORD("get_height", FV(fix_tilts));
    return pos.x < 0 ? NO_HEIGHT : 0;
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

ITEM_ACTION_SLOT ItemAction_IDToSlot(const ITEM_ACTION_ID action)
{
    return (ITEM_ACTION_SLOT)action;
}

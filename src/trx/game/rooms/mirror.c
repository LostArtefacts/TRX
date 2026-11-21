#include <trx/game/rooms/mirror.h>

#include <trx/game/rooms/const.h>

static int16_t m_RoomNum = NO_ROOM;
static int32_t m_ZPlane = 0;

void Room_ResetMirror(void)
{
    m_RoomNum = NO_ROOM;
    m_ZPlane = 0;
}

void Room_SetMirror(const int16_t room_num, const int32_t z_plane)
{
    m_RoomNum = room_num;
    m_ZPlane = z_plane;
}

bool Room_IsMirror(const int16_t room_num)
{
    return m_RoomNum != NO_ROOM && room_num == m_RoomNum;
}

int32_t Room_GetMirrorPlane(void)
{
    return m_ZPlane;
}

XYZ_32 Room_ReflectInMirror(const XYZ_32 pos)
{
    return (XYZ_32) { .x = pos.x, .y = pos.y, .z = 2 * m_ZPlane - pos.z };
}

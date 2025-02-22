#include "game/camera/common.h"

#include "game/camera/vars.h"
#include "game/rooms/const.h"

static bool m_IsChunky = false;

bool Camera_IsChunky(void)
{
    return m_IsChunky;
}

void Camera_SetChunky(const bool is_chunky)
{
    m_IsChunky = is_chunky;
}

void Camera_Reset(void)
{
    g_Camera.pos.room_num = NO_ROOM;
}

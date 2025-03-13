#include "game/camera/common.h"

#include "game/camera/vars.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "utils.h"

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

void Camera_ClampInterpResult(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Room_GetSector(
            g_Camera.interp.result.pos.x,
            g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
            g_Camera.interp.result.pos.z, &g_Camera.interp.room_num);
        return;
    }

    XYZ_32 *const pos = &g_Camera.interp.result.pos;
    const int32_t shift = g_Camera.interp.result.shift;
    const ROOM *const room = Room_Get(g_Camera.interp.room_num);
    const SECTOR *sector = Room_GetWorldSector(room, pos->x, pos->z);
    if (sector->box != NO_BOX) {
        goto finish;
    }

    sector = Room_GetWorldSector(room, g_Camera.pos.x, g_Camera.pos.z);
    if (sector->box == NO_BOX) {
        goto finish;
    }

    const BOX_INFO *const box = Box_GetBox(sector->box);
    CLAMP(pos->x, box->top, box->bottom);
    CLAMP(pos->z, box->left, box->right);

finish:
    const int32_t floor = Room_GetHeight(sector, pos->x, pos->y, pos->z);
    const int32_t ceiling = Room_GetCeiling(sector, pos->x, pos->y, pos->z);
    if (floor != NO_HEIGHT && ceiling != NO_HEIGHT) {
        CLAMP(pos->y, ceiling - shift, floor - shift);
    }
    Room_GetSector(pos->x, pos->y + shift, pos->z, &g_Camera.interp.room_num);
}

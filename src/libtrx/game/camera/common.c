#include "game/camera/common.h"

#include "game/camera/vars.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "utils.h"

static BOX_INFO m_FixedBox = {};
static bool m_IsChunky = false;

// TODO: make private once modules are ported.
const BOX_INFO *Camera_GetBox(
    const SECTOR *const sector, const int32_t x, const int32_t z)
{
    if (sector->box != NO_BOX) {
        return Box_GetBox(sector->box);
    }

    // A level may have blocked specific sector or room pathfinding, so create a
    // dummy one-sector box to prevent erratic camera positioning.
    m_FixedBox.left = z & ~(WALL_L - 1);
    m_FixedBox.top = x & ~(WALL_L - 1);
    m_FixedBox.right = m_FixedBox.left + WALL_L - 1;
    m_FixedBox.bottom = m_FixedBox.top + WALL_L - 1;
    return &m_FixedBox;
}

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
    const int32_t floor =
        Room_GetHeightEx(sector, pos->x, pos->y, pos->z, true);
    const int32_t ceiling =
        Room_GetCeilingEx(sector, pos->x, pos->y, pos->z, true);
    if (floor != NO_HEIGHT && ceiling != NO_HEIGHT) {
        CLAMP(pos->y, ceiling - shift, floor - shift);
    }
    Room_GetSector(pos->x, pos->y + shift, pos->z, &g_Camera.interp.room_num);
}

void Camera_RefreshFromTrigger(const TRIGGER *const trigger)
{
    int16_t target_ok = 2;

    const TRIGGER_CMD *cmd = trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CAMERA) {
            const TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == g_Camera.last) {
                g_Camera.num = cam_data->camera_num;

                if (g_Camera.timer < 0 || g_Camera.type == CAM_LOOK
                    || g_Camera.type == CAM_COMBAT) {
                    g_Camera.timer = -1;
                    target_ok = 0;
                } else {
                    g_Camera.type = CAM_FIXED;
                    target_ok = 1;
                }
            } else {
                target_ok = 0;
            }
        } else if (cmd->type == TO_TARGET) {
            if (g_Camera.type != CAM_LOOK && g_Camera.type != CAM_COMBAT) {
                g_Camera.item = Item_Get((int16_t)(intptr_t)cmd->parameter);
            }
        }
    }

    if (g_Camera.item != nullptr
        && (target_ok == 0
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item))) {
        g_Camera.item = nullptr;
    }

    if (g_Camera.num == -1 && g_Camera.timer > 0) {
        g_Camera.timer = -1;
    }
}

#include "game/camera.h"

#include "game/los.h"
#include "game/output.h"
#include "game/random.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

void Camera_LoadCutsceneFrame(void)
{
    CINE_DATA *const cine_data = Camera_GetCineData();
    if (cine_data->frame_count == 0) {
        return;
    }

    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        cine_data->frame_idx = cine_data->frame_count - 1;
    }

    const CINE_FRAME *const frame = Camera_GetCurrentCineFrame();
    int32_t tx = frame->tx;
    int32_t ty = frame->ty;
    int32_t tz = frame->tz;
    int32_t cx = frame->cx;
    int32_t cy = frame->cy;
    int32_t cz = frame->cz;
    int32_t fov = frame->fov;
    int32_t roll = frame->roll;
    int32_t c = Math_Cos(cine_data->position.rot.y);
    int32_t s = Math_Sin(cine_data->position.rot.y);

    g_Camera.target.x =
        cine_data->position.pos.x + ((tx * c + tz * s) >> W2V_SHIFT);
    g_Camera.target.y = cine_data->position.pos.y + ty;
    g_Camera.target.z =
        cine_data->position.pos.z + ((tz * c - tx * s) >> W2V_SHIFT);
    g_Camera.pos.x =
        cine_data->position.pos.x + ((cx * c + cz * s) >> W2V_SHIFT);
    g_Camera.pos.y = cine_data->position.pos.y + cy;
    g_Camera.pos.z =
        cine_data->position.pos.z + ((cz * c - cx * s) >> W2V_SHIFT);
    g_Camera.roll = roll;
    g_Camera.shift = 0;

    const int16_t room_num =
        Room_GetIndexFromPos(g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }

    Viewport_AlterFOV(fov);
    Camera_UpdateMicPosition();
}

void Camera_UpdateCutscene(void)
{
    if (Camera_GetCineData()->frame_count == 0) {
        return;
    }

    const CINE_FRAME *const frame = Camera_GetCurrentCineFrame();
    int32_t tx = frame->tx;
    int32_t ty = frame->ty;
    int32_t tz = frame->tz;
    int32_t cx = frame->cx;
    int32_t cy = frame->cy;
    int32_t cz = frame->cz;
    int32_t fov = frame->fov;
    int32_t roll = frame->roll;
    int32_t c = Math_Cos(g_Camera.target_angle);
    int32_t s = Math_Sin(g_Camera.target_angle);
    const XYZ_32 camera_target = {
        .x = g_LaraItem->pos.x + ((tx * c + tz * s) >> W2V_SHIFT),
        .y = g_LaraItem->pos.y + ty,
        .z = g_LaraItem->pos.z + ((tz * c - tx * s) >> W2V_SHIFT),
    };
    const XYZ_32 camera_pos = {
        .x = g_LaraItem->pos.x + ((cx * c + cz * s) >> W2V_SHIFT),
        .y = g_LaraItem->pos.y + cy,
        .z = g_LaraItem->pos.z + ((cz * c - cx * s) >> W2V_SHIFT),
    };
    const int16_t room_num =
        Room_GetIndexFromPos(camera_pos.x, camera_pos.y, camera_pos.z);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }

    g_Camera.pos.pos = camera_pos;
    g_Camera.target.pos = camera_target;
    g_Camera.roll = roll;
    g_Camera.shift = 0;
    Viewport_AlterFOV(fov);
}

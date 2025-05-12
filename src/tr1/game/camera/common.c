#include "game/camera/common.h"

#include "game/input.h"
#include "game/los.h"
#include "game/random.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>

void Camera_LoadCutsceneFrame(void)
{
    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        cine_data->frame_idx = cine_data->frame_count - 1;
    }

    Camera_UpdateCutscene();
}

void Camera_UpdateCutscene(void)
{
    const CINE_DATA *const cine_data = Camera_GetCineData();
    if (cine_data->frame_count == 0) {
        return;
    }

    const CINE_FRAME *const ref = Camera_GetCurrentCineFrame();
    const int32_t c = Math_Cos(cine_data->position.rot.y);
    const int32_t s = Math_Sin(cine_data->position.rot.y);
    const XYZ_32 *const pos = &cine_data->position.pos;
    g_Camera.target.x = pos->x + ((c * ref->tx + s * ref->tz) >> W2V_SHIFT);
    g_Camera.target.y = pos->y + ref->ty;
    g_Camera.target.z = pos->z + ((c * ref->tz - s * ref->tx) >> W2V_SHIFT);
    g_Camera.pos.x = pos->x + ((s * ref->cz + c * ref->cx) >> W2V_SHIFT);
    g_Camera.pos.y = pos->y + ref->cy;
    g_Camera.pos.z = pos->z + ((c * ref->cz - s * ref->cx) >> W2V_SHIFT);
    const int16_t room_num =
        Room_GetIndexFromPos(g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }
    g_Camera.roll = ref->roll;
    g_Camera.shift = 0;

    Viewport_SetFOV(ref->fov);
}

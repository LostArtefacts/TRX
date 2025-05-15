#include "game/camera/cinematic.h"

#include "game/camera.h"
#include "game/game_buf.h"
#include "game/lara.h"
#include "game/rooms.h"
#include "game/viewport.h"

static CINE_FRAME *m_CineFrames = nullptr;
static CINE_DATA m_CineData = {};

void Camera_InitialiseCineFrames(const int32_t num_frames)
{
    m_CineData.frame_count = num_frames;
    m_CineData.frame_idx = 0;
    m_CineFrames = num_frames == 0
        ? nullptr
        : GameBuf_Alloc(num_frames * sizeof(CINE_FRAME), GBUF_CINEMATIC_FRAMES);
}

CINE_FRAME *Camera_GetCineFrame(const int32_t frame_idx)
{
    if (m_CineFrames == nullptr) {
        return nullptr;
    }
    return &m_CineFrames[frame_idx];
}

CINE_FRAME *Camera_GetCurrentCineFrame(void)
{
    return Camera_GetCineFrame(m_CineData.frame_idx);
}

CINE_DATA *Camera_GetCineData(void)
{
    return &m_CineData;
}

void Camera_InvokeCinematic(
    const ITEM *const item, const int32_t frame_idx, const int16_t extra_y_rot)
{
    g_Camera.type = CAM_CINEMATIC;
    m_CineData.frame_idx = frame_idx;
    m_CineData.position.pos = item->pos;
    m_CineData.position.rot = item->rot;
    m_CineData.position.rot.y += extra_y_rot;
}

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

#if TR_VERSION == 1
    Camera_UpdateCutscene();
#else
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
#endif
}

void Camera_UpdateCutscene(void)
{
    const CINE_DATA *const cine_data = Camera_GetCineData();
    if (cine_data->frame_count == 0) {
        return;
    }

    const CINE_FRAME *const frame = Camera_GetCurrentCineFrame();
#if TR_VERSION == 1
    const int32_t c = Math_Cos(cine_data->position.rot.y);
    const int32_t s = Math_Sin(cine_data->position.rot.y);
    const XYZ_32 *const pos = &cine_data->position.pos;
    g_Camera.target.x = pos->x + ((c * frame->tx + s * frame->tz) >> W2V_SHIFT);
    g_Camera.target.y = pos->y + frame->ty;
    g_Camera.target.z = pos->z + ((c * frame->tz - s * frame->tx) >> W2V_SHIFT);
    g_Camera.pos.x = pos->x + ((s * frame->cz + c * frame->cx) >> W2V_SHIFT);
    g_Camera.pos.y = pos->y + frame->cy;
    g_Camera.pos.z = pos->z + ((c * frame->cz - s * frame->cx) >> W2V_SHIFT);
    const int16_t room_num =
        Room_GetIndexFromPos(g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }
    g_Camera.roll = frame->roll;
    g_Camera.shift = 0;

    Viewport_SetFOV(frame->fov);
#else
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

    const ITEM *const lara_item = Lara_GetItem();
    const XYZ_32 camera_target = {
        .x = lara_item->pos.x + ((tx * c + tz * s) >> W2V_SHIFT),
        .y = lara_item->pos.y + ty,
        .z = lara_item->pos.z + ((tz * c - tx * s) >> W2V_SHIFT),
    };
    const XYZ_32 camera_pos = {
        .x = lara_item->pos.x + ((cx * c + cz * s) >> W2V_SHIFT),
        .y = lara_item->pos.y + cy,
        .z = lara_item->pos.z + ((cz * c - cx * s) >> W2V_SHIFT),
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
#endif
}

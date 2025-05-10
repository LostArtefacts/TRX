#include "game/camera/common.h"

#include "config.h"
#include "debug.h"
#include "game/camera/const.h"
#include "game/camera/vars.h"
#include "game/lara.h"
#include "game/los.h"
#include "game/matrix.h"
#include "game/music.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "utils.h"

#if TR_VERSION == 2
// TODO: consolidate with Viewport API
extern int32_t g_PhdPersp;
#endif

static BOX_INFO m_FixedBox = {};
static bool m_IsChunky = false;

static void M_AdjustMusicVolume(bool underwater);
static void M_EnsureEnvironment(void);

static void M_AdjustMusicVolume(const bool underwater)
{
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
    double multiplier = 1.0;

    if (underwater) {
        switch (g_Config.audio.underwater_music_mode) {
        case UMM_QUIET:
            multiplier = 0.5;
            break;
        case UMM_NONE:
            multiplier = 0.0;
            break;
        case UMM_FULL_NO_AMBIENT:
            multiplier = is_ambient ? 0.0 : 1.0;
            break;
        case UMM_QUIET_NO_AMBIENT:
            multiplier = is_ambient ? 0.0 : 0.5;
            break;
        case UMM_FULL:
        default:
            multiplier = 1.0;
            break;
        }
    }

    Music_SetVolume(g_Config.audio.music_volume * multiplier);
}

static void M_EnsureEnvironment(void)
{
    if (g_Camera.pos.room_num == NO_ROOM) {
        return;
    }

    const ROOM *const room = Room_Get(g_Camera.pos.room_num);
    if ((room->flags & RF_UNDERWATER) != 0) {
        M_AdjustMusicVolume(true);
        Sound_Effect(SFX_UNDERWATER, nullptr, SPM_ALWAYS);
        g_Camera.underwater = true;
    } else {
        M_AdjustMusicVolume(false);
        if (g_Camera.underwater) {
            Sound_StopEffect(SFX_UNDERWATER);
            g_Camera.underwater = false;
        }
    }
}

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

// TODO: make private.
bool Camera_IsGoodPosition(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    return Camera_GetSector(x, y, z, room_num) != nullptr;
}

// TODO: make private.
const SECTOR *Camera_GetSector(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t height = Room_GetHeight(sector, x, y, z);
    const int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    if (y > height || y < ceiling) {
        return nullptr;
    }

    return sector;
}

// TODO: make private.
void Camera_Move(const GAME_VECTOR *const target, const int32_t speed)
{
    const GAME_VECTOR old_pos = g_Camera.pos;
    GAME_VECTOR pos = g_Camera.pos;
    pos.x += (target->x - pos.x) / speed;
    pos.z += (target->z - pos.z) / speed;
    pos.y += (target->y - pos.y) / speed;
    pos.room_num = target->room_num;

    Camera_SetChunky(false);

    const SECTOR *sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
    int32_t height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
    if (height == NO_HEIGHT) {
        // Attempt to clamp within the previous sector's height bounds. Only if
        // that fails continue to revert fully to the last good Y position.
        pos.room_num = old_pos.room_num;
        sector = Room_GetSector(old_pos.x, old_pos.y, old_pos.z, &pos.room_num);
        height = Room_GetHeight(sector, old_pos.x, old_pos.y, old_pos.z);
        const int32_t old_ceiling =
            Room_GetCeiling(sector, old_pos.x, old_pos.y, old_pos.z);
        CLAMP(pos.y, old_ceiling + STEP_L, height - STEP_L);
        sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
        height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
        if (height == NO_HEIGHT) {
            pos.y = old_pos.y;
            pos.room_num = old_pos.room_num;
            sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
            height = Room_GetHeight(sector, pos.x, pos.y, pos.z);
        }
    }

    height -= STEP_L;
    if (pos.y >= height && target->y >= height) {
        LOS_Check(&g_Camera.target, &pos);
        sector = Room_GetSector(pos.x, pos.y, pos.z, &pos.room_num);
        height = Room_GetHeight(sector, pos.x, pos.y, pos.z) - STEP_L;
    }

    g_Camera.pos = pos;

    int32_t ceiling = Room_GetCeiling(sector, pos.x, pos.y, pos.z) + STEP_L;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (g_Camera.bounce > 0) {
        g_Camera.pos.y += g_Camera.bounce;
        g_Camera.target.y += g_Camera.bounce;
        g_Camera.bounce = 0;
    } else if (g_Camera.bounce < 0) {
        const XYZ_32 shake = {
            .x = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
            .y = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
            .z = g_Camera.bounce * (Random_GetControl() - 0x4000) / 0x7FFF,
        };
        g_Camera.pos.x += shake.x;
        g_Camera.pos.y += shake.y;
        g_Camera.pos.z += shake.z;
        g_Camera.target.y += shake.x;
        g_Camera.target.y += shake.y;
        g_Camera.target.z += shake.z;
        g_Camera.bounce += 5;
    }

    if (g_Camera.pos.y > height) {
        g_Camera.shift = height - g_Camera.pos.y;
    } else if (g_Camera.pos.y < ceiling) {
        g_Camera.shift = ceiling - g_Camera.pos.y;
    } else {
        g_Camera.shift = 0;
    }

#if TR_VERSION == 2
    if (g_Config.audio.enable_lara_mic) {
        const LARA_INFO *const lara_info = Lara_GetLaraInfo();
        const ITEM *const lara_item = Lara_GetItem();
        g_Camera.actual_angle =
            lara_info->torso_rot.y + lara_info->head_rot.y + lara_item->rot.y;
        g_Camera.mic_pos = lara_item->pos;
    } else {
        g_Camera.actual_angle = Math_Atan(
            g_Camera.target.z - g_Camera.pos.z,
            g_Camera.target.x - g_Camera.pos.x);
        g_Camera.mic_pos.x = g_Camera.pos.x
            + ((g_PhdPersp * Math_Sin(g_Camera.actual_angle)) >> W2V_SHIFT);
        g_Camera.mic_pos.z = g_Camera.pos.z
            + ((g_PhdPersp * Math_Cos(g_Camera.actual_angle)) >> W2V_SHIFT);
        g_Camera.mic_pos.y = g_Camera.pos.y;
    }
#endif
}

bool Camera_IsChunky(void)
{
    return m_IsChunky;
}

void Camera_SetChunky(const bool is_chunky)
{
    m_IsChunky = is_chunky;
}

void Camera_Initialise(void)
{
    Matrix_ResetStack();
    g_Camera.underwater = false;
    g_Camera.last = NO_CAMERA;
    Camera_ResetPosition();
#if TR_VERSION == 2
    Viewport_AlterFOV(-1);
#endif
    Camera_Update();
}

void Camera_ResetPosition(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    ASSERT(lara_item != nullptr);
    g_Camera.shift = lara_item->pos.y - WALL_L;

    g_Camera.target.x = lara_item->pos.x;
    g_Camera.target.y = g_Camera.shift;
    g_Camera.target.z = lara_item->pos.z;
    g_Camera.target.room_num = lara_item->room_num;

    g_Camera.pos.x = g_Camera.target.x;
    g_Camera.pos.y = g_Camera.target.y;
    g_Camera.pos.z = g_Camera.target.z - 100;
    g_Camera.pos.room_num = g_Camera.target.room_num;

    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.item = nullptr;

    g_Camera.speed = 1;
    g_Camera.flags = CF_NORMAL;
    g_Camera.bounce = 0;
    g_Camera.num = NO_CAMERA;
    g_Camera.fixed_camera = false;
#if TR_VERSION == 1
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
    g_Camera.type = CAM_CHASE;
#else
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!lara_info->extra_anim) {
        g_Camera.type = CAM_CHASE;
    }
#endif
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
#if TR_VERSION == 1
    const bool fix_glide_cameras = false;
#else
    const bool fix_glide_cameras = g_Config.visuals.fix_glide_cameras;
#endif

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
                    if (fix_glide_cameras && cam_data->glide != 0) {
                        g_Camera.speed = cam_data->glide + 1;
                    }
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
#if TR_VERSION >= 2
    // TODO: check if this can be removed from TR2 during camera module merge.
    // It is required not to be present in TR1 otherwise heavy triggers (that
    // don't have camera data) can cancel active cameras triggered by Lara.
    if (g_Camera.num == -1 && g_Camera.timer > 0) {
        g_Camera.timer = -1;
    }
#endif
}

void Camera_Apply(void)
{
    M_EnsureEnvironment();
    Matrix_LookAt(
        g_Camera.interp.result.pos.x,
        g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
        g_Camera.interp.result.pos.z, g_Camera.interp.result.target.x,
        g_Camera.interp.result.target.y, g_Camera.interp.result.target.z,
        g_Camera.roll);
}

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
int32_t Camera_ShiftClamp(GAME_VECTOR *const pos, const int32_t clamp)
{
    const int32_t x = pos->x;
    const int32_t y = pos->y;
    const int32_t z = pos->z;

    const SECTOR *const sector = Room_GetSector(x, y, z, &pos->room_num);
    const BOX_INFO *const box = Camera_GetBox(sector, x, z);

    const int32_t left = box->left + clamp;
    const int32_t right = box->right - clamp;
    if (z < left && !Camera_IsGoodPosition(x, y, z - clamp, pos->room_num)) {
        pos->z = left;
    } else if (
        z > right && !Camera_IsGoodPosition(x, y, z + clamp, pos->room_num)) {
        pos->z = right;
    }

    const int32_t top = box->top + clamp;
    const int32_t bottom = box->bottom - clamp;
    if (x < top && !Camera_IsGoodPosition(x - clamp, y, z, pos->room_num)) {
        pos->x = top;
    } else if (
        x > bottom && !Camera_IsGoodPosition(x + clamp, y, z, pos->room_num)) {
        pos->x = bottom;
    }

    int32_t height = Room_GetHeight(sector, x, y, z) - clamp;
    int32_t ceiling = Room_GetCeiling(sector, x, y, z) + clamp;

    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (y > height) {
        return height - y;
    } else if (y < ceiling) {
        return ceiling - y;
    }

    return 0;
}

// TODO: make private
void Camera_SmartShift(
    GAME_VECTOR *const target, void (*shift)(CAMERA_SHIFT_ARGS))
{
    LOS_Check(&g_Camera.target, target);

    const ROOM *room = Room_Get(g_Camera.target.room_num);
    const SECTOR *sector =
        Room_GetWorldSector(room, g_Camera.target.x, g_Camera.target.z);
    const BOX_INFO *box =
        Camera_GetBox(sector, g_Camera.target.x, g_Camera.target.z);

    room = Room_Get(target->room_num);
    sector = Room_GetWorldSector(room, target->x, target->z);

    if (target->z < box->left || target->z > box->right || target->x < box->top
        || target->x > box->bottom) {
        box = Camera_GetBox(sector, target->x, target->z);
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    int32_t test = (target->z - WALL_L) | (WALL_L - 1);
    const SECTOR *const good_left =
        Camera_GetSector(target->x, target->y, test, target->room_num);
    if (good_left != nullptr) {
        box = Camera_GetBox(good_left, target->x, test);
        if (box->left < left) {
            left = box->left;
        }
    } else if (TR_VERSION == 2) {
        left = test;
    }

    test = (target->z + WALL_L) & (~(WALL_L - 1));
    const SECTOR *const good_right =
        Camera_GetSector(target->x, target->y, test, target->room_num);
    if (good_right != nullptr) {
        box = Camera_GetBox(good_right, target->x, test);
        if (box->right > right) {
            right = box->right;
        }
    } else if (TR_VERSION == 2) {
        right = test;
    }

    test = (target->x - WALL_L) | (WALL_L - 1);
    const SECTOR *const good_top =
        Camera_GetSector(test, target->y, target->z, target->room_num);
    if (good_top != nullptr) {
        box = Camera_GetBox(good_top, test, target->z);
        if (box->top < top) {
            top = box->top;
        }
    } else if (TR_VERSION == 2) {
        top = test;
    }

    test = (target->x + WALL_L) & (~(WALL_L - 1));
    const SECTOR *const good_bottom =
        Camera_GetSector(test, target->y, target->z, target->room_num);
    if (good_bottom != nullptr) {
        box = Camera_GetBox(good_bottom, test, target->z);
        if (box->bottom > bottom) {
            bottom = box->bottom;
        }
    } else if (TR_VERSION == 2) {
        bottom = test;
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    GAME_VECTOR target_a = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    GAME_VECTOR target_b = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    bool clip = false;
    bool prefer_a = true;

#define SHIFT(axis1, axis2, l1, l2, r1, r2)                                    \
    shift(                                                                     \
        &target_a.axis1, &target_a.axis2, &target_a.y, g_Camera.target.axis1,  \
        g_Camera.target.axis2, g_Camera.target.y, l1, l2, r1, r2);             \
    shift(                                                                     \
        &target_b.axis1, &target_b.axis2, &target_b.y, g_Camera.target.axis1,  \
        g_Camera.target.axis2, g_Camera.target.y, l1, r2, r1, l2)

#if TR_VERSION == 1
    if (target->z < left && good_left == nullptr) {
        clip = true;
        if (target->x < g_Camera.target.x) {
            SHIFT(z, x, left, top, right, bottom);
        } else {
            SHIFT(z, x, left, bottom, right, top);
        }
    } else if (target->z > right && good_right == nullptr) {
        clip = true;
        if (target->x < g_Camera.target.x) {
            SHIFT(z, x, right, top, left, bottom);
        } else {
            SHIFT(z, x, right, bottom, left, top);
        }
    } else if (target->x < top && good_top == nullptr) {
        clip = true;
        if (target->z < g_Camera.target.z) {
            SHIFT(x, z, top, left, bottom, right);
        } else {
            SHIFT(x, z, top, right, bottom, left);
        }
    } else if (target->x > bottom && good_bottom == nullptr) {
        clip = true;
        if (target->z < g_Camera.target.z) {
            SHIFT(x, z, bottom, left, top, right);
        } else {
            SHIFT(x, z, bottom, right, top, left);
        }
    }
#else
    if (ABS(target->z - g_Camera.target.z)
        > ABS(target->x - g_Camera.target.x)) {
        if (target->z < left && good_left == nullptr) {
            clip = true;
            prefer_a = g_Camera.pos.x < g_Camera.target.x;
            SHIFT(z, x, left, top, right, bottom);
        } else if (target->z > right && good_right == nullptr) {
            clip = true;
            prefer_a = g_Camera.pos.x < g_Camera.target.x;
            SHIFT(z, x, right, top, left, bottom);
        } else if (target->x < top && good_top == nullptr) {
            clip = true;
            prefer_a = target->z < g_Camera.target.z;
            SHIFT(x, z, top, left, bottom, right);
        } else if (target->x > bottom && good_bottom == nullptr) {
            clip = true;
            prefer_a = target->z < g_Camera.target.z;
            SHIFT(x, z, bottom, left, top, right);
        }
    } else {
        if (target->x < top && good_top == nullptr) {
            clip = true;
            prefer_a = g_Camera.pos.z < g_Camera.target.z;
            SHIFT(x, z, top, left, bottom, right);
        } else if (target->x > bottom && good_bottom == nullptr) {
            clip = true;
            prefer_a = g_Camera.pos.z < g_Camera.target.z;
            SHIFT(x, z, bottom, left, top, right);
        } else if (target->z < left && good_left == nullptr) {
            clip = true;
            prefer_a = target->x < g_Camera.target.x;
            SHIFT(z, x, left, top, right, bottom);
        } else if (target->z > right && good_right == nullptr) {
            clip = true;
            prefer_a = target->x < g_Camera.target.x;
            SHIFT(z, x, right, top, left, bottom);
        }
    }
#endif

#undef SHIFT

    if (!clip) {
        return;
    }
#if TR_VERSION == 2
    if (prefer_a) {
        prefer_a = LOS_Check(&g_Camera.target, &target_a);
    } else {
        prefer_a = !LOS_Check(&g_Camera.target, &target_b);
    }
#endif
    if (prefer_a) {
        target->pos = target_a.pos;
    } else {
        target->pos = target_b.pos;
    }

    Room_GetSector(target->x, target->y, target->z, &target->room_num);
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

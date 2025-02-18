#include "game/camera.h"

#include "game/items.h"
#include "game/los.h"
#include "game/music.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define CHASE_SPEED 10
#define CHASE_ELEVATION (WALL_L * 3 / 2) // = 1536

#define COMBAT_SPEED 8
#define COMBAT_DISTANCE (WALL_L * 5 / 2) // = 2560

#define LOOK_DISTANCE (WALL_L * 3 / 2) // = 1536
#define LOOK_CLAMP (STEP_L + 50) // = 296
#define LOOK_SPEED 4

#define MAX_ELEVATION (85 * DEG_1) // = 15470

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

void Camera_Initialise(void)
{
    Matrix_ResetStack();
    g_Camera.last = NO_CAMERA;
    Camera_ResetPosition();
    Viewport_AlterFOV(-1);
    Camera_Update();
}

void Camera_ResetPosition(void)
{
    ASSERT(g_LaraItem != nullptr);
    g_Camera.shift = g_LaraItem->pos.y - WALL_L;

    g_Camera.target.x = g_LaraItem->pos.x;
    g_Camera.target.y = g_Camera.shift;
    g_Camera.target.z = g_LaraItem->pos.z;
    g_Camera.target.room_num = g_LaraItem->room_num;

    g_Camera.pos.x = g_Camera.target.x;
    g_Camera.pos.y = g_Camera.target.y;
    g_Camera.pos.z = g_Camera.target.z - 100;
    g_Camera.pos.room_num = g_Camera.target.room_num;

    g_Camera.target_distance = WALL_L * 3 / 2;
    g_Camera.item = nullptr;

    if (!g_Lara.extra_anim) {
        g_Camera.type = CAM_CHASE;
    }

    g_Camera.speed = 1;
    g_Camera.flags = CF_NORMAL;
    g_Camera.bounce = 0;
    g_Camera.num = NO_CAMERA;
    g_Camera.fixed_camera = false;
}

void Camera_Move(const GAME_VECTOR *target, int32_t speed)
{
    g_Camera.pos.x += (target->x - g_Camera.pos.x) / speed;
    g_Camera.pos.z += (target->z - g_Camera.pos.z) / speed;
    g_Camera.pos.y += (target->y - g_Camera.pos.y) / speed;
    g_Camera.pos.room_num = target->room_num;

    Camera_SetChunky(false);

    const SECTOR *sector = Room_GetSector(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z, &g_Camera.pos.room_num);
    int32_t height =
        Room_GetHeight(sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        - STEP_L;

    if (g_Camera.pos.y >= height && target->y >= height) {
        LOS_Check(&g_Camera.target, &g_Camera.pos);
        sector = Room_GetSector(
            g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
            &g_Camera.pos.room_num);
        height = Room_GetHeight(
                     sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
            - STEP_L;
    }

    int32_t ceiling =
        Room_GetCeiling(sector, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        + STEP_L;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (g_Camera.bounce > 0) {
        g_Camera.pos.y += g_Camera.bounce;
        g_Camera.target.y += g_Camera.bounce;
        g_Camera.bounce = 0;
    } else if (g_Camera.bounce < 0) {
        XYZ_32 shake = {
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

    Room_GetSector(
        g_Camera.pos.x, g_Camera.pos.y + g_Camera.shift, g_Camera.pos.z,
        &g_Camera.pos.room_num);

    if (g_Config.audio.enable_lara_mic) {
        g_Camera.actual_angle =
            g_Lara.torso_rot.y + g_Lara.head_rot.y + g_LaraItem->rot.y;
        g_Camera.mic_pos.x = g_LaraItem->pos.x;
        g_Camera.mic_pos.y = g_LaraItem->pos.y;
        g_Camera.mic_pos.z = g_LaraItem->pos.z;
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
}

void Camera_Clip(
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
    int32_t target_h, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    if ((right > left) != (target_x < left)) {
        *y = target_y + (left - target_x) * (*y - target_y) / (*x - target_x);
        *h = target_h + (left - target_x) * (*h - target_h) / (*x - target_x);
        *x = left;
    }

    if ((bottom > top && target_y > top && (*y) < top)
        || (bottom < top && target_y < top && (*y) > top)) {
        *x = target_x + (top - target_y) * (*x - target_x) / (*y - target_y);
        *h = target_h + (top - target_y) * (*h - target_h) / (*y - target_y);
        *y = top;
    }
}

void Camera_Shift(
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
    int32_t target_h, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    int32_t shift;

    int32_t l_square = SQUARE(target_x - left);
    int32_t r_square = SQUARE(target_x - right);
    int32_t t_square = SQUARE(target_y - top);
    int32_t b_square = SQUARE(target_y - bottom);
    int32_t tl_square = t_square + l_square;
    int32_t tr_square = t_square + r_square;
    int32_t bl_square = b_square + l_square;

    if (g_Camera.target_square < tl_square) {
        *x = left;
        shift = g_Camera.target_square - l_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *y = target_y + (top >= bottom ? shift : -shift);
        }
    } else if (tl_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (g_Camera.target_square < bl_square) {
        *x = left;
        shift = g_Camera.target_square - l_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *y = target_y + (top < bottom ? shift : -shift);
        }
    } else if (2 * g_Camera.target_square < tr_square) {
        shift = 2 * g_Camera.target_square - t_square;
        if (shift >= 0) {
            shift = Math_Sqrt(shift);
            *x = target_x + (left < right ? shift : -shift);
            *y = top;
        }
    } else if (bl_square <= tr_square) {
        *x = right;
        *y = top;
    } else {
        *x = left;
        *y = bottom;
    }
}

const SECTOR *Camera_GoodPosition(
    int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    int32_t ceiling = Room_GetCeiling(sector, x, y, z);
    if (y > height || y < ceiling) {
        return nullptr;
    }

    return sector;
}

void Camera_SmartShift(
    GAME_VECTOR *target,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
        int32_t target_h, int32_t left, int32_t top, int32_t right,
        int32_t bottom))
{
    LOS_Check(&g_Camera.target, target);

    const ROOM *room = Room_Get(g_Camera.target.room_num);
    int16_t item_box =
        Room_GetWorldSector(room, g_Camera.target.x, g_Camera.target.z)->box;
    const BOX_INFO *box = Box_GetBox(item_box);

    room = Room_Get(target->room_num);
    int16_t camera_box = Room_GetWorldSector(room, target->x, target->z)->box;

    if (camera_box != NO_BOX
        && (target->z < box->left || target->z > box->right
            || target->x < box->top || target->x > box->bottom)) {
        box = Box_GetBox(camera_box);
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    int32_t test;

    test = (target->z - WALL_L) | (WALL_L - 1);
    const SECTOR *good_left =
        Camera_GoodPosition(target->x, target->y, test, target->room_num);
    if (good_left) {
        camera_box = good_left->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->left < left) {
            left = Box_GetBox(camera_box)->left;
        }
    } else {
        left = test;
    }

    test = (target->z + WALL_L) & (~(WALL_L - 1));
    const SECTOR *good_right =
        Camera_GoodPosition(target->x, target->y, test, target->room_num);
    if (good_right) {
        camera_box = good_right->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->right > right) {
            right = Box_GetBox(camera_box)->right;
        }
    } else {
        right = test;
    }

    test = (target->x - WALL_L) | (WALL_L - 1);
    const SECTOR *good_top =
        Camera_GoodPosition(test, target->y, target->z, target->room_num);
    if (good_top) {
        camera_box = good_top->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->top < top) {
            top = Box_GetBox(camera_box)->top;
        }
    } else {
        top = test;
    }

    test = (target->x + WALL_L) & (~(WALL_L - 1));
    const SECTOR *good_bottom =
        Camera_GoodPosition(test, target->y, target->z, target->room_num);
    if (good_bottom) {
        camera_box = good_bottom->box;
        if (camera_box != NO_BOX && Box_GetBox(camera_box)->bottom > bottom) {
            bottom = Box_GetBox(camera_box)->bottom;
        }
    } else {
        bottom = test;
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    GAME_VECTOR a = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    GAME_VECTOR b = {
        .x = target->x,
        .y = target->y,
        .z = target->z,
        .room_num = target->room_num,
    };

    int32_t noclip = 1;
    int32_t prefer_a;

    if (ABS(target->z - g_Camera.target.z)
        > ABS(target->x - g_Camera.target.x)) {
        if (target->z < left && !good_left) {
            noclip = 0;
            prefer_a = g_Camera.pos.x < g_Camera.target.x;
            shift(
                &a.z, &a.x, &a.y, g_Camera.target.z, g_Camera.target.x,
                g_Camera.target.y, left, top, right, bottom);
            shift(
                &b.z, &b.x, &b.y, g_Camera.target.z, g_Camera.target.x,
                g_Camera.target.y, left, bottom, right, top);
        } else if (target->z > right && !good_right) {
            noclip = 0;
            prefer_a = g_Camera.pos.x < g_Camera.target.x;
            shift(
                &a.z, &a.x, &a.y, g_Camera.target.z, g_Camera.target.x,
                g_Camera.target.y, right, top, left, bottom);
            shift(
                &b.z, &b.x, &b.y, g_Camera.target.z, g_Camera.target.x,
                g_Camera.target.y, right, bottom, left, top);
        }

        if (noclip) {
            if (target->x < top && !good_top) {
                noclip = 0;
                prefer_a = target->z < g_Camera.target.z;
                shift(
                    &a.x, &a.z, &a.y, g_Camera.target.x, g_Camera.target.z,
                    g_Camera.target.y, top, left, bottom, right);
                shift(
                    &b.x, &b.z, &b.y, g_Camera.target.x, g_Camera.target.z,
                    g_Camera.target.y, top, right, bottom, left);
            } else if (target->x > bottom && !good_bottom) {
                noclip = 0;
                prefer_a = target->z < g_Camera.target.z;
                shift(
                    &a.x, &a.z, &a.y, g_Camera.target.x, g_Camera.target.z,
                    g_Camera.target.y, bottom, left, top, right);
                shift(
                    &b.x, &b.z, &b.y, g_Camera.target.x, g_Camera.target.z,
                    g_Camera.target.y, bottom, right, top, left);
            }
        }
    } else {
        if (target->x < top && !good_top) {
            noclip = 0;
            prefer_a = g_Camera.pos.z < g_Camera.target.z;
            shift(
                &a.x, &a.z, &a.y, g_Camera.target.x, g_Camera.target.z,
                g_Camera.target.y, top, left, bottom, right);
            shift(
                &b.x, &b.z, &b.y, g_Camera.target.x, g_Camera.target.z,
                g_Camera.target.y, top, right, bottom, left);
        } else if (target->x > bottom && !good_bottom) {
            noclip = 0;
            prefer_a = g_Camera.pos.z < g_Camera.target.z;
            shift(
                &a.x, &a.z, &a.y, g_Camera.target.x, g_Camera.target.z,
                g_Camera.target.y, bottom, left, top, right);
            shift(
                &b.x, &b.z, &b.y, g_Camera.target.x, g_Camera.target.z,
                g_Camera.target.y, bottom, right, top, left);
        }

        if (noclip) {
            if (target->z < left && !good_left) {
                noclip = 0;
                prefer_a = target->x < g_Camera.target.x;
                shift(
                    &a.z, &a.x, &a.y, g_Camera.target.z, g_Camera.target.x,
                    g_Camera.target.y, left, top, right, bottom);
                shift(
                    &b.z, &b.x, &b.y, g_Camera.target.z, g_Camera.target.x,
                    g_Camera.target.y, left, bottom, right, top);
            } else if (target->z > right && !good_right) {
                noclip = 0;
                prefer_a = target->x < g_Camera.target.x;
                shift(
                    &a.z, &a.x, &a.y, g_Camera.target.z, g_Camera.target.x,
                    g_Camera.target.y, right, top, left, bottom);
                shift(
                    &b.z, &b.x, &b.y, g_Camera.target.z, g_Camera.target.x,
                    g_Camera.target.y, right, bottom, left, top);
            }
        }
    }

    if (noclip) {
        return;
    }

    if (prefer_a) {
        prefer_a = LOS_Check(&g_Camera.target, &a) != 0;
    } else {
        prefer_a = LOS_Check(&g_Camera.target, &b) == 0;
    }

    if (prefer_a) {
        target->x = a.x;
        target->y = a.y;
        target->z = a.z;
    } else {
        target->x = b.x;
        target->y = b.y;
        target->z = b.z;
    }

    Room_GetSector(target->x, target->y, target->z, &target->room_num);
}

void Camera_Chase(const ITEM *item)
{
    g_Camera.target_elevation += item->rot.x;
    g_Camera.target_elevation = MIN(g_Camera.target_elevation, MAX_ELEVATION);
    g_Camera.target_elevation = MAX(g_Camera.target_elevation, -MAX_ELEVATION);

    int32_t distance =
        (g_Camera.target_distance * Math_Cos(g_Camera.target_elevation))
        >> W2V_SHIFT;
    int16_t angle = g_Camera.target_angle + item->rot.y;

    g_Camera.target_square = SQUARE(distance);

    const XYZ_32 offset = {
        .y = (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
            >> W2V_SHIFT,
        .x = -((distance * Math_Sin(angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    Camera_SmartShift(&target, Camera_Shift);
    Camera_Move(&target, g_Camera.speed);
}

int32_t Camera_ShiftClamp(GAME_VECTOR *pos, int32_t clamp)
{
    int32_t x = pos->x;
    int32_t y = pos->y;
    int32_t z = pos->z;
    const SECTOR *const sector = Room_GetSector(x, y, z, &pos->room_num);
    const BOX_INFO *const box = Box_GetBox(sector->box);

    const int32_t left = box->left + clamp;
    const int32_t right = box->right - clamp;
    if (z < left && !Camera_GoodPosition(x, y, z - clamp, pos->room_num)) {
        pos->z = left;
    } else if (
        z > right && !Camera_GoodPosition(x, y, z + clamp, pos->room_num)) {
        pos->z = right;
    }

    const int32_t top = box->top + clamp;
    const int32_t bottom = box->bottom - clamp;
    if (x < top && !Camera_GoodPosition(x - clamp, y, z, pos->room_num)) {
        pos->x = top;
    } else if (
        x > bottom && !Camera_GoodPosition(x + clamp, y, z, pos->room_num)) {
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
    }
    if (y < ceiling) {
        return ceiling - y;
    }
    return 0;
}

void Camera_Combat(const ITEM *item)
{
    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    g_Camera.target_distance = COMBAT_DISTANCE;
    if (g_Lara.target) {
        g_Camera.target_angle = g_Lara.target_angles[0] + item->rot.y;
        g_Camera.target_elevation = g_Lara.target_angles[1] + item->rot.x;
    } else {
        g_Camera.target_angle =
            g_Lara.torso_rot.y + g_Lara.head_rot.y + item->rot.y;
        g_Camera.target_elevation =
            g_Lara.torso_rot.x + g_Lara.head_rot.x + item->rot.x;
    }

    int32_t distance =
        (COMBAT_DISTANCE * Math_Cos(g_Camera.target_elevation)) >> W2V_SHIFT;
    int16_t angle = g_Camera.target_angle;

    const XYZ_32 offset = {
        .y =
            +((g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
              >> W2V_SHIFT),
        .x = -((distance * Math_Sin(angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    if (g_Lara.water_status == LWS_UNDERWATER) {
        int32_t water_height = g_Lara.water_surface_dist + g_LaraItem->pos.y;
        if (g_Camera.target.y > water_height && water_height > target.y) {
            target.y = g_Lara.water_surface_dist + g_LaraItem->pos.y;
            target.z = g_Camera.target.z
                + (water_height - g_Camera.target.y)
                    * (target.z - g_Camera.target.z)
                    / (target.y - g_Camera.target.y);
            target.x = g_Camera.target.x
                + (water_height - g_Camera.target.y)
                    * (target.x - g_Camera.target.x)
                    / (target.y - g_Camera.target.y);
        }
    }

    Camera_SmartShift(&target, Camera_Shift);
    Camera_Move(&target, g_Camera.speed);
}

void Camera_Look(const ITEM *item)
{
    XYZ_32 old = {
        .x = g_Camera.target.x,
        .y = g_Camera.target.y,
        .z = g_Camera.target.z,
    };

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;
    g_Camera.target_angle =
        item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    g_Camera.target_distance = LOOK_DISTANCE;
    g_Camera.target_elevation =
        g_Lara.torso_rot.x + g_Lara.head_rot.x + item->rot.x;

    int32_t distance =
        (LOOK_DISTANCE * Math_Cos(g_Camera.target_elevation)) >> W2V_SHIFT;

    g_Camera.shift =
        (-STEP_L * 2 * Math_Sin(g_Camera.target_elevation)) >> W2V_SHIFT;
    g_Camera.target.z += (g_Camera.shift * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    g_Camera.target.x += (g_Camera.shift * Math_Sin(item->rot.y)) >> W2V_SHIFT;

    if (!Camera_GoodPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_num)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += Camera_ShiftClamp(&g_Camera.target, LOOK_CLAMP);

    const XYZ_32 offset = {
        .y =
            +((g_Camera.target_distance * Math_Sin(g_Camera.target_elevation))
              >> W2V_SHIFT),
        .x = -((distance * Math_Sin(g_Camera.target_angle)) >> W2V_SHIFT),
        .z = -((distance * Math_Cos(g_Camera.target_angle)) >> W2V_SHIFT),
    };

    GAME_VECTOR target = {
        .x = g_Camera.target.x + offset.x,
        .y = g_Camera.target.y + offset.y,
        .z = g_Camera.target.z + offset.z,
        .room_num = g_Camera.pos.room_num,
    };

    Camera_SmartShift(&target, Camera_Clip);
    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;
    Camera_Move(&target, g_Camera.speed);
    g_Camera.debuff = 5;
}

void Camera_Fixed(void)
{
    const OBJECT_VECTOR *const fixed = Camera_GetFixedObject(g_Camera.num);
    GAME_VECTOR target = {
        .x = fixed->x,
        .y = fixed->y,
        .z = fixed->z,
        .room_num = fixed->data,
    };
    if (!LOS_Check(&g_Camera.target, &target)) {
        Camera_ShiftClamp(&target, STEP_L);
    }

    g_Camera.fixed_camera = true;
    Camera_Move(&target, g_Camera.speed);

    if (g_Camera.timer) {
        g_Camera.timer--;
        if (!g_Camera.timer) {
            g_Camera.timer = -1;
        }
    }
}

void Camera_Update(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Camera_UpdatePhotoMode();
        return;
    }

    if (g_Camera.type == CAM_CINEMATIC) {
        Camera_LoadCutsceneFrame();
        return;
    }

    if (g_Camera.flags != CF_NO_CHUNKY) {
        Camera_SetChunky(true);
    }

    const bool fixed_camera = g_Camera.item != nullptr
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    const ITEM *const item = fixed_camera ? g_Camera.item : g_LaraItem;

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (fixed_camera) {
        y += (bounds->min.y + bounds->max.y) / 2;
    } else {
        y += bounds->max.y
            + (((int32_t)(bounds->min.y - bounds->max.y)) * 3 >> 2);
    }

    if (g_Camera.item && !fixed_camera) {
        bounds = Item_GetBoundsAccurate(g_Camera.item);

        const int32_t dx = g_Camera.item->pos.x - item->pos.x;
        const int32_t dz = g_Camera.item->pos.z - item->pos.z;
        const int32_t shift = Math_Sqrt(SQUARE(dx) + SQUARE(dz));
        int16_t angle = Math_Atan(dz, dx) - item->rot.y;

        int16_t tilt = Math_Atan(
            shift,
            y - (bounds->min.y + bounds->max.y) / 2 - g_Camera.item->pos.y);
        angle >>= 1;
        tilt >>= 1;

        if (angle > MIN_HEAD_ROTATION_CAM && angle < MAX_HEAD_ROTATION_CAM
            && tilt > MIN_HEAD_TILT_CAM && tilt < MAX_HEAD_TILT_CAM) {
            int16_t change = angle - g_Lara.head_rot.y;
            if (change > HEAD_TURN_CAM) {
                g_Lara.head_rot.y += HEAD_TURN_CAM;
            } else if (change < -HEAD_TURN_CAM) {
                g_Lara.head_rot.y -= HEAD_TURN_CAM;
            } else {
                g_Lara.head_rot.y = angle;
            }

            change = tilt - g_Lara.head_rot.x;
            if (change > HEAD_TURN_CAM) {
                g_Lara.head_rot.x += HEAD_TURN_CAM;
            } else if (change < -HEAD_TURN_CAM) {
                g_Lara.head_rot.x -= HEAD_TURN_CAM;
            } else {
                g_Lara.head_rot.x += change;
            }
            g_Lara.torso_rot.x = g_Lara.head_rot.x;
            g_Lara.torso_rot.y = g_Lara.head_rot.y;
            g_Camera.type = CAM_LOOK;
            g_Camera.item->looked_at = 1;
        }
    }

    if (g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT) {
        y -= STEP_L;
        g_Camera.target.room_num = item->room_num;
        if (g_Camera.fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) >> 2;
            g_Camera.speed =
                g_Camera.type == CAM_LOOK ? LOOK_SPEED : COMBAT_SPEED;
        }
        g_Camera.fixed_camera = false;
        if (g_Camera.type == CAM_LOOK) {
            Camera_Look(item);
        } else {
            Camera_Combat(item);
        }
    } else {
        if (g_Camera.debuff > 0) {
            const XYZ_32 old = g_Camera.target.pos;
            g_Camera.target.x = (item->pos.x + old.x) / 2;
            g_Camera.target.z = (item->pos.z + old.z) / 2;
            g_Camera.debuff--;
        } else {
            g_Camera.target.x = item->pos.x;
            g_Camera.target.z = item->pos.z;
        }

        if (g_Camera.flags == CF_FOLLOW_CENTRE) {
            const int32_t shift = (bounds->min.z + bounds->max.z) / 2;
            g_Camera.target.z += (shift * Math_Cos(item->rot.y)) >> W2V_SHIFT;
            g_Camera.target.x += (shift * Math_Sin(item->rot.y)) >> W2V_SHIFT;
        }

        g_Camera.target.room_num = item->room_num;
        if (g_Camera.fixed_camera != fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.fixed_camera = true;
            g_Camera.speed = 1;
        } else {
            g_Camera.fixed_camera = false;
            g_Camera.target.y += (y - g_Camera.target.y) / 4;
        }

        const SECTOR *const sector = Room_GetSector(
            g_Camera.target.x, y, g_Camera.target.z, &g_Camera.target.room_num);
        const int32_t height = Room_GetHeight(
            sector, g_Camera.target.x, g_Camera.target.y, g_Camera.target.z);
        if (g_Camera.target.y > height) {
            Camera_SetChunky(false);
        }

        if (g_Camera.type == CAM_CHASE || g_Camera.flags == CF_CHASE_OBJECT) {
            Camera_Chase(item);
        } else {
            Camera_Fixed();
        }
    }

    g_Camera.last = g_Camera.num;
    g_Camera.fixed_camera = fixed_camera;
    if (g_Camera.type != CAM_HEAVY || g_Camera.timer == -1) {
        g_Camera.type = CAM_CHASE;
        g_Camera.speed = CHASE_SPEED;
        g_Camera.num = NO_CAMERA;
        g_Camera.last_item = g_Camera.item;
        g_Camera.item = nullptr;
        g_Camera.target_elevation = 0;
        g_Camera.target_angle = 0;
        g_Camera.target_distance = CHASE_ELEVATION;
        g_Camera.flags = CF_NORMAL;
    }
    Camera_SetChunky(false);
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

    Viewport_AlterFOV(fov);

    Room_GetSector(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z, &g_Camera.pos.room_num);

    if (g_Config.audio.enable_lara_mic) {
        g_Camera.actual_angle =
            g_Lara.torso_rot.y + g_Lara.head_rot.y + g_LaraItem->rot.y;
        g_Camera.mic_pos.x = g_LaraItem->pos.x;
        g_Camera.mic_pos.y = g_LaraItem->pos.y;
        g_Camera.mic_pos.z = g_LaraItem->pos.z;
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
    int16_t room_num = Room_FindByPos(camera_pos.x, camera_pos.y, camera_pos.z);
    if (room_num != NO_ROOM_NEG) {
        g_Camera.pos.room_num = room_num;
    }

    g_Camera.pos.pos = camera_pos;
    g_Camera.target.pos = camera_target;
    g_Camera.roll = roll;
    g_Camera.shift = 0;
    Viewport_AlterFOV(fov);
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

    if (g_Camera.item
        && (!target_ok
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item))) {
        g_Camera.item = nullptr;
    }

    if (g_Camera.num == -1 && g_Camera.timer > 0) {
        g_Camera.timer = -1;
    }
}

void Camera_Apply(void)
{
    M_EnsureEnvironment();
    Matrix_LookAt(
        g_Camera.pos.x, g_Camera.pos.y + g_Camera.shift, g_Camera.pos.z,
        g_Camera.target.x, g_Camera.target.y, g_Camera.target.z, g_Camera.roll);
}

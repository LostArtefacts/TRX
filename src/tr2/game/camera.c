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

#define CHASE_SPEED 10
#define CHASE_ELEVATION (WALL_L * 3 / 2) // = 1536

#define COMBAT_SPEED 8
#define COMBAT_DISTANCE (WALL_L * 5 / 2) // = 2560

#define LOOK_DISTANCE (WALL_L * 3 / 2) // = 1536
#define LOOK_CLAMP (STEP_L + 50) // = 296
#define LOOK_SPEED 4

#define MAX_ELEVATION (85 * DEG_1) // = 15470

void Camera_Clip(CAMERA_SHIFT_ARGS)
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

void Camera_Shift(CAMERA_SHIFT_ARGS)
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

    if (!Camera_IsGoodPosition(
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

    const int16_t room_num =
        Room_GetIndexFromPos(g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }

    Viewport_AlterFOV(fov);

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

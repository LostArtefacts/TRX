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

// Camera speed option ranges from 1-10, so index 0 is unused.
static double m_ManualCameraMultiplier[11] = {
    1.0, .5, .625, .75, .875, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0,
};

static void M_Chase(const ITEM *item);
static void M_Combat(const ITEM *item);
static void M_Look(const ITEM *item);
static void M_Fixed(void);
static void M_LoadCutsceneFrame(void);

static void M_OffsetAdditionalAngle(int16_t delta);
static void M_OffsetAdditionalElevation(int16_t delta);
static void M_OffsetReset(void);

static void M_Shift(CAMERA_SHIFT_ARGS);

static void M_Chase(const ITEM *const item)
{
    GAME_VECTOR ideal;

    g_Camera.target_elevation += item->rot.x;
    if (g_Camera.target_elevation > MAX_ELEVATION) {
        g_Camera.target_elevation = MAX_ELEVATION;
    } else if (g_Camera.target_elevation < -MAX_ELEVATION) {
        g_Camera.target_elevation = -MAX_ELEVATION;
    }

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);

    g_Camera.target_square = SQUARE(distance);

    const PHD_ANGLE angle = item->rot.y + g_Camera.target_angle;
    ideal.x = g_Camera.target.x - (distance * Math_Sin(angle) >> W2V_SHIFT);
    ideal.z = g_Camera.target.z - (distance * Math_Cos(angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    Camera_SmartShift(&ideal, M_Shift);

    if (g_Camera.fixed_camera) {
        Camera_Move(&ideal, g_Camera.speed);
    } else {
        Camera_Move(&ideal, CHASE_SPEED);
    }
}

static void M_Combat(const ITEM *const item)
{
    GAME_VECTOR ideal;

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    if (g_Lara.target) {
        g_Camera.target_angle = item->rot.y + g_Lara.target_angles[0];
        g_Camera.target_elevation = item->rot.x + g_Lara.target_angles[1];
    } else {
        g_Camera.target_angle =
            item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
        g_Camera.target_elevation =
            item->rot.x + g_Lara.torso_rot.x + g_Lara.head_rot.x;
    }

    g_Camera.target_distance = COMBAT_DISTANCE;

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    Camera_SmartShift(&ideal, M_Shift);
    Camera_Move(&ideal, g_Camera.speed);
}

static void M_Look(const ITEM *const item)
{
    const GAME_VECTOR old = {
        .x = g_Camera.target.x,
        .z = g_Camera.target.z,
    };

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    g_Camera.target_angle =
        item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    g_Camera.target_elevation =
        item->rot.x + g_Lara.torso_rot.x + g_Lara.head_rot.x;
    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;

    const int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    g_Camera.shift =
        -STEP_L * 2 * Math_Sin(g_Camera.target_elevation) >> W2V_SHIFT;
    g_Camera.target.z += g_Camera.shift * Math_Cos(item->rot.y) >> W2V_SHIFT;
    g_Camera.target.x += g_Camera.shift * Math_Sin(item->rot.y) >> W2V_SHIFT;

    if (!Camera_IsGoodPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_num)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += Camera_ShiftClamp(&g_Camera.target, STEP_L + 50);

    GAME_VECTOR ideal;
    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_num = g_Camera.pos.room_num;

    Camera_SmartShift(&ideal, Camera_Clip);

    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;

    Camera_Move(&ideal, g_Camera.speed);
    g_Camera.debuff = 5;
}

static void M_Fixed(void)
{
    const OBJECT_VECTOR *const fixed = Camera_GetFixedObject(g_Camera.num);
    GAME_VECTOR ideal = {
        .x = fixed->x,
        .y = fixed->y,
        .z = fixed->z,
        .room_num = fixed->data,
    };

    g_Camera.fixed_camera = true;

    Camera_Move(&ideal, g_Camera.speed);

    if (g_Camera.timer) {
        g_Camera.timer--;
        if (!g_Camera.timer) {
            g_Camera.timer = -1;
        }
    }
}

static void M_LoadCutsceneFrame(void)
{
    CINE_DATA *const cine_data = Camera_GetCineData();
    cine_data->frame_idx++;
    if (cine_data->frame_idx >= cine_data->frame_count) {
        cine_data->frame_idx = cine_data->frame_count - 1;
    }

    Camera_UpdateCutscene();
}

static void M_OffsetAdditionalAngle(const int16_t delta)
{
    g_Camera.additional_angle += delta;
}

static void M_OffsetAdditionalElevation(const int16_t delta)
{
    // don't let this value wrap, so clamp it.
    if (delta > 0) {
        if (g_Camera.additional_elevation > INT16_MAX - delta) {
            g_Camera.additional_elevation = INT16_MAX;
        } else {
            g_Camera.additional_elevation += delta;
        }
    } else {
        if (g_Camera.additional_elevation < INT16_MIN - delta) {
            g_Camera.additional_elevation = INT16_MIN;
        } else {
            g_Camera.additional_elevation += delta;
        }
    }
}

static void M_OffsetReset(void)
{
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

static void M_Shift(CAMERA_SHIFT_ARGS)
{
    const int32_t tl_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    const int32_t bl_square =
        SQUARE(target_x - left) + SQUARE(target_y - bottom);
    const int32_t tr_square = SQUARE(target_x - right) + SQUARE(target_y - top);

    int32_t shift;
    if (g_Camera.target_square < tl_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *y = target_y + ((top < bottom) ? -shift : shift);
    } else if (tl_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (g_Camera.target_square < bl_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *y = target_y + ((top < bottom) ? shift : -shift);
    } else if (bl_square > MIN_SQUARE) {
        *x = left;
        *y = bottom;
    } else if (g_Camera.target_square < tr_square) {
        shift = g_Camera.target_square - SQUARE(target_y - top);
        if (shift < 0) {
            return;
        }

        shift = Math_Sqrt(shift);
        *x = target_x + ((left < right) ? shift : -shift);
        *y = top;
    } else {
        *x = right;
        *y = top;
    }
}

void Camera_Update(void)
{
    if (g_Camera.type == CAM_PHOTO_MODE) {
        Camera_UpdatePhotoMode();
        return;
    }

    if (g_Camera.type == CAM_CINEMATIC) {
        M_LoadCutsceneFrame();
        return;
    }

    if (g_Camera.flags != CF_NO_CHUNKY) {
        Camera_SetChunky(true);
    }

    const bool fixed_camera = g_Camera.item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    const ITEM *const item = fixed_camera ? g_Camera.item : g_LaraItem;

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (!fixed_camera) {
        y += bounds->max.y + ((bounds->min.y - bounds->max.y) * 3 >> 2);
    } else {
        y += (bounds->min.y + bounds->max.y) / 2;
    }

    if (g_Camera.item && !fixed_camera) {
        bounds = Item_GetBoundsAccurate(g_Camera.item);
        const int16_t shift = Math_Sqrt(
            SQUARE(g_Camera.item->pos.z - item->pos.z)
            + SQUARE(g_Camera.item->pos.x - item->pos.x));
        int16_t angle = Math_Atan(
                            g_Camera.item->pos.z - item->pos.z,
                            g_Camera.item->pos.x - item->pos.x)
            - item->rot.y;
        int16_t tilt = Math_Atan(
            shift,
            y - (g_Camera.item->pos.y + (bounds->min.y + bounds->max.y) / 2));
        angle >>= 1;
        tilt >>= 1;

        if (angle > -MAX_HEAD_ROTATION && angle < MAX_HEAD_ROTATION
            && tilt > MIN_HEAD_TILT_CAM && tilt < MAX_HEAD_TILT_CAM) {
            int16_t change = angle - g_Lara.head_rot.y;
            if (change > HEAD_TURN) {
                g_Lara.head_rot.y += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_rot.y -= HEAD_TURN;
            } else {
                g_Lara.head_rot.y += change;
            }

            change = tilt - g_Lara.head_rot.x;
            if (change > HEAD_TURN) {
                g_Lara.head_rot.x += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_rot.x -= HEAD_TURN;
            } else {
                g_Lara.head_rot.x += change;
            }

            g_Lara.torso_rot.y = g_Lara.head_rot.y;
            g_Lara.torso_rot.x = g_Lara.head_rot.x;

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
            M_Look(item);
        } else {
            M_Combat(item);
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
            const int16_t shift = (bounds->min.z + bounds->max.z) / 2;
            g_Camera.target.z += Math_Cos(item->rot.y) * shift >> W2V_SHIFT;
            g_Camera.target.x += Math_Sin(item->rot.y) * shift >> W2V_SHIFT;
        }

        g_Camera.target.room_num = item->room_num;

        if (g_Camera.fixed_camera != fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.fixed_camera = true;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) / 4;
            g_Camera.fixed_camera = false;
        }

        const SECTOR *const sector = Room_GetSector(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            &g_Camera.target.room_num);
        if (g_Camera.target.y > Room_GetHeight(
                sector, g_Camera.target.x, g_Camera.target.y,
                g_Camera.target.z)) {
            Camera_SetChunky(false);
        }

        if (g_Camera.type == CAM_CHASE || g_Camera.flags == CF_CHASE_OBJECT) {
            M_Chase(item);
        } else {
            M_Fixed();
        }
    }

    g_Camera.last = g_Camera.num;
    g_Camera.fixed_camera = fixed_camera;

    // should we clear the manual camera
    switch (g_Camera.type) {
    case CAM_LOOK:
    case CAM_CINEMATIC:
    case CAM_COMBAT:
    case CAM_FIXED:
        g_Camera.additional_angle = 0;
        g_Camera.additional_elevation = 0;
        break;

    default:
        break;
    }

    if (g_Camera.type != CAM_HEAVY || g_Camera.timer == -1) {
        g_Camera.type = CAM_CHASE;
        g_Camera.num = NO_CAMERA;
        g_Camera.last_item = g_Camera.item;
        g_Camera.item = nullptr;
        g_Camera.target_angle = g_Camera.additional_angle;
        g_Camera.target_elevation = g_Camera.additional_elevation;
        g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
        g_Camera.flags = CF_NORMAL;
    }

    Camera_SetChunky(false);
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

void Camera_MoveManual(void)
{
    const int16_t camera_delta = (const int)DEG_90 / (const int)LOGIC_FPS
        * (double)m_ManualCameraMultiplier[g_Config.gameplay.camera_speed];

    if (g_Input.camera_left) {
        M_OffsetAdditionalAngle(camera_delta);
    } else if (g_Input.camera_right) {
        M_OffsetAdditionalAngle(-camera_delta);
    }
    if (g_Input.camera_forward) {
        M_OffsetAdditionalElevation(-camera_delta);
    } else if (g_Input.camera_back) {
        M_OffsetAdditionalElevation(camera_delta);
    }
    if (g_Input.camera_reset) {
        M_OffsetReset();
    }
}

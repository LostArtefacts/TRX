#include "game/camera.h"

#include "config.h"
#include "game/input.h"
#include "game/items.h"
#include "game/los.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"

#include <libtrx/utils.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

// Camera speed option ranges from 1-10, so index 0 is unused.
static double m_ManualCameraMultiplier[11] = {
    1.0, .5, .625, .75, .875, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0,
};

static bool Camera_BadPosition(
    int32_t x, int32_t y, int32_t z, int16_t room_num);
static int32_t Camera_ShiftClamp(GAME_VECTOR *pos, int32_t clamp);
static void Camera_SmartShift(
    GAME_VECTOR *ideal,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom));
static void Camera_Clip(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);
static void Camera_Shift(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom);
static void Camera_Move(GAME_VECTOR *ideal, int32_t speed);
static void Camera_LoadCutsceneFrame(void);
static void Camera_OffsetAdditionalAngle(int16_t delta);
static void Camera_OffsetAdditionalElevation(int16_t delta);
static void Camera_EnsureEnvironment(void);

static bool Camera_BadPosition(
    int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    return y >= Room_GetHeight(floor, x, y, z)
        || y <= Room_GetCeiling(floor, x, y, z);
}

static int32_t Camera_ShiftClamp(GAME_VECTOR *pos, int32_t clamp)
{
    int32_t x = pos->x;
    int32_t y = pos->y;
    int32_t z = pos->z;

    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &pos->room_number);

    BOX_INFO *box = &g_Boxes[floor->box];
    if (z < box->left + clamp
        && Camera_BadPosition(x, y, z - clamp, pos->room_number)) {
        pos->z = box->left + clamp;
    } else if (
        z > box->right - clamp
        && Camera_BadPosition(x, y, z + clamp, pos->room_number)) {
        pos->z = box->right - clamp;
    }

    if (x < box->top + clamp
        && Camera_BadPosition(x - clamp, y, z, pos->room_number)) {
        pos->x = box->top + clamp;
    } else if (
        x > box->bottom - clamp
        && Camera_BadPosition(x + clamp, y, z, pos->room_number)) {
        pos->x = box->bottom - clamp;
    }

    int32_t height = Room_GetHeight(floor, x, y, z) - clamp;
    int32_t ceiling = Room_GetCeiling(floor, x, y, z) + clamp;

    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (y > height) {
        return height - y;
    } else if (pos->y < ceiling) {
        return ceiling - y;
    } else {
        return 0;
    }
}

static void Camera_SmartShift(
    GAME_VECTOR *ideal,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom))
{
    LOS_Check(&g_Camera.target, ideal);

    ROOM_INFO *r = &g_RoomInfo[g_Camera.target.room_number];
    int32_t x_floor = (g_Camera.target.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (g_Camera.target.x - r->x) >> WALL_SHIFT;

    int16_t item_box = r->floor[x_floor + y_floor * r->x_size].box;
    BOX_INFO *box = &g_Boxes[item_box];

    r = &g_RoomInfo[ideal->room_number];
    x_floor = (ideal->z - r->z) >> WALL_SHIFT;
    y_floor = (ideal->x - r->x) >> WALL_SHIFT;

    int16_t camera_box = r->floor[x_floor + y_floor * r->x_size].box;
    if (camera_box != NO_BOX
        && (ideal->z < box->left || ideal->z > box->right || ideal->x < box->top
            || ideal->x > box->bottom)) {
        box = &g_Boxes[camera_box];
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    int32_t test = (ideal->z - WALL_L) | (WALL_L - 1);
    bool bad_left =
        Camera_BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_left) {
        camera_box = r->floor[x_floor - 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].left < left) {
            left = g_Boxes[camera_box].left;
        }
    }

    test = (ideal->z + WALL_L) & (~(WALL_L - 1));
    bool bad_right =
        Camera_BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_right) {
        camera_box = r->floor[x_floor + 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].right > right) {
            right = g_Boxes[camera_box].right;
        }
    }

    test = (ideal->x - WALL_L) | (WALL_L - 1);
    bool bad_top =
        Camera_BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_top) {
        camera_box = r->floor[x_floor + (y_floor - 1) * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].top < top) {
            top = g_Boxes[camera_box].top;
        }
    }

    test = (ideal->x + WALL_L) & (~(WALL_L - 1));
    bool bad_bottom =
        Camera_BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_bottom) {
        camera_box = r->floor[x_floor + (y_floor + 1) * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].bottom > bottom) {
            bottom = g_Boxes[camera_box].bottom;
        }
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    int32_t noclip = 1;
    if (ideal->z < left && bad_left) {
        noclip = 0;
        if (ideal->x < g_Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                left, top, right, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                left, bottom, right, top);
        }
    } else if (ideal->z > right && bad_right) {
        noclip = 0;
        if (ideal->x < g_Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                right, top, left, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, g_Camera.target.z, g_Camera.target.x,
                right, bottom, left, top);
        }
    }

    if (noclip) {
        if (ideal->x < top && bad_top) {
            noclip = 0;
            if (ideal->z < g_Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    top, left, bottom, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    top, right, bottom, left);
            }
        } else if (ideal->x > bottom && bad_bottom) {
            noclip = 0;
            if (ideal->z < g_Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    bottom, left, top, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, g_Camera.target.x, g_Camera.target.z,
                    bottom, right, top, left);
            }
        }
    }

    if (!noclip) {
        Room_GetFloor(ideal->x, ideal->y, ideal->z, &ideal->room_number);
    }
}

static void Camera_Clip(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    if ((right > left) != (target_x < left)) {
        *y = target_y + (*y - target_y) * (left - target_x) / (*x - target_x);
        *x = left;
    }

    if ((bottom > top && target_y > top && *y < top)
        || (bottom < top && target_y < top && (*y) > top)) {
        *x = target_x + (*x - target_x) * (top - target_y) / (*y - target_y);
        *y = top;
    }
}

static void Camera_Shift(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    int32_t shift;

    int32_t tl_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    int32_t bl_square = SQUARE(target_x - left) + SQUARE(target_y - bottom);
    int32_t tr_square = SQUARE(target_x - right) + SQUARE(target_y - top);

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

static void Camera_Move(GAME_VECTOR *ideal, int32_t speed)
{
    g_Camera.pos.x += (ideal->x - g_Camera.pos.x) / speed;
    g_Camera.pos.z += (ideal->z - g_Camera.pos.z) / speed;
    g_Camera.pos.y += (ideal->y - g_Camera.pos.y) / speed;
    g_Camera.pos.room_number = ideal->room_number;

    g_ChunkyFlag = false;

    FLOOR_INFO *floor = Room_GetFloor(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
        &g_Camera.pos.room_number);
    int32_t height =
        Room_GetHeight(floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        - GROUND_SHIFT;

    if (g_Camera.pos.y >= height && ideal->y >= height) {
        LOS_Check(&g_Camera.target, &g_Camera.pos);
        floor = Room_GetFloor(
            g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
            &g_Camera.pos.room_number);
        height = Room_GetHeight(
                     floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
            - GROUND_SHIFT;
    }

    int32_t ceiling =
        Room_GetCeiling(floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        + GROUND_SHIFT;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (g_Camera.bounce) {
        if (g_Camera.bounce > 0) {
            g_Camera.pos.y += g_Camera.bounce;
            g_Camera.target.y += g_Camera.bounce;
            g_Camera.bounce = 0;
        } else {
            int32_t shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.x += shake;
            g_Camera.target.y += shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.y += shake;
            g_Camera.target.y += shake;
            shake = (Random_GetControl() - 0x4000) * g_Camera.bounce / 0x7FFF;
            g_Camera.pos.z += shake;
            g_Camera.target.z += shake;
            g_Camera.bounce += 5;
        }
    }

    if (g_Camera.pos.y > height) {
        g_Camera.shift = height - g_Camera.pos.y;
    } else if (g_Camera.pos.y < ceiling) {
        g_Camera.shift = ceiling - g_Camera.pos.y;
    } else {
        g_Camera.shift = 0;
    }
}

static void Camera_LoadCutsceneFrame(void)
{
    g_CineFrame++;
    if (g_CineFrame >= g_NumCineFrames) {
        g_CineFrame = g_NumCineFrames - 1;
    }

    CINE_CAMERA *ref = &g_CineCamera[g_CineFrame];

    int32_t c = Math_Cos(g_CinePosition.rot.y);
    int32_t s = Math_Sin(g_CinePosition.rot.y);

    g_Camera.target.x =
        g_CinePosition.pos.x + ((c * ref->tx + s * ref->tz) >> W2V_SHIFT);
    g_Camera.target.y = g_CinePosition.pos.y + ref->ty;
    g_Camera.target.z =
        g_CinePosition.pos.z + ((c * ref->tz - s * ref->tx) >> W2V_SHIFT);
    g_Camera.pos.x =
        g_CinePosition.pos.x + ((s * ref->cz + c * ref->cx) >> W2V_SHIFT);
    g_Camera.pos.y = g_CinePosition.pos.y + ref->cy;
    g_Camera.pos.z =
        g_CinePosition.pos.z + ((c * ref->cz - s * ref->cx) >> W2V_SHIFT);
    g_Camera.roll = ref->roll;
    g_Camera.shift = 0;

    Viewport_SetFOV(ref->fov);
}

static void Camera_OffsetAdditionalAngle(int16_t delta)
{
    g_Camera.additional_angle += delta;
}

static void Camera_OffsetAdditionalElevation(int16_t delta)
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

void Camera_Reset(void)
{
    g_Camera.pos.room_number = NO_ROOM;
}

void Camera_ResetPosition(void)
{
    assert(g_LaraItem);
    g_Camera.shift = g_LaraItem->pos.y - WALL_L;

    g_Camera.target.x = g_LaraItem->pos.x;
    g_Camera.target.y = g_Camera.shift;
    g_Camera.target.z = g_LaraItem->pos.z;
    g_Camera.target.room_number = g_LaraItem->room_number;

    g_Camera.pos.x = g_Camera.target.x;
    g_Camera.pos.y = g_Camera.target.y;
    g_Camera.pos.z = g_Camera.target.z - 100;
    g_Camera.pos.room_number = g_Camera.target.room_number;

    g_Camera.target_distance = WALL_L * 3 / 2;
    g_Camera.item = NULL;

    g_Camera.type = CAM_CHASE;
    g_Camera.flags = 0;
    g_Camera.bounce = 0;
    g_Camera.number = NO_CAMERA;
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

void Camera_Initialise(void)
{
    Camera_ResetPosition();
    Camera_Update();
}

void Camera_Chase(ITEM_INFO *item)
{
    GAME_VECTOR ideal;

    g_Camera.target_elevation += item->rot.x;
    if (g_Camera.target_elevation > MAX_ELEVATION) {
        g_Camera.target_elevation = MAX_ELEVATION;
    } else if (g_Camera.target_elevation < -MAX_ELEVATION) {
        g_Camera.target_elevation = -MAX_ELEVATION;
    }

    int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);

    g_Camera.target_square = SQUARE(distance);

    PHD_ANGLE angle = item->rot.y + g_Camera.target_angle;
    ideal.x = g_Camera.target.x - (distance * Math_Sin(angle) >> W2V_SHIFT);
    ideal.z = g_Camera.target.z - (distance * Math_Cos(angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    Camera_SmartShift(&ideal, Camera_Shift);

    if (g_Camera.fixed_camera) {
        Camera_Move(&ideal, g_Camera.speed);
    } else {
        Camera_Move(&ideal, CHASE_SPEED);
    }
}

void Camera_Combat(ITEM_INFO *item)
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

    int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    Camera_SmartShift(&ideal, Camera_Shift);
    Camera_Move(&ideal, g_Camera.speed);
}

void Camera_Look(ITEM_INFO *item)
{
    GAME_VECTOR old;
    GAME_VECTOR ideal;

    old.z = g_Camera.target.z;
    old.x = g_Camera.target.x;

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    g_Camera.target_angle =
        item->rot.y + g_Lara.torso_rot.y + g_Lara.head_rot.y;
    g_Camera.target_elevation =
        item->rot.x + g_Lara.torso_rot.x + g_Lara.head_rot.x;
    g_Camera.target_distance = WALL_L * 3 / 2;

    int32_t distance =
        g_Camera.target_distance * Math_Cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    g_Camera.shift =
        -STEP_L * 2 * Math_Sin(g_Camera.target_elevation) >> W2V_SHIFT;
    g_Camera.target.z += g_Camera.shift * Math_Cos(item->rot.y) >> W2V_SHIFT;
    g_Camera.target.x += g_Camera.shift * Math_Sin(item->rot.y) >> W2V_SHIFT;

    if (Camera_BadPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_number)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += Camera_ShiftClamp(&g_Camera.target, STEP_L + 50);

    ideal.x = g_Camera.target.x
        - (distance * Math_Sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * Math_Sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * Math_Cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    Camera_SmartShift(&ideal, Camera_Clip);

    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;

    Camera_Move(&ideal, g_Camera.speed);
}

void Camera_Fixed(void)
{
    GAME_VECTOR ideal;
    OBJECT_VECTOR *fixed;

    fixed = &g_Camera.fixed[g_Camera.number];
    ideal.x = fixed->x;
    ideal.y = fixed->y;
    ideal.z = fixed->z;
    ideal.room_number = fixed->data;

    g_Camera.fixed_camera = 1;

    Camera_Move(&ideal, g_Camera.speed);

    if (g_Camera.timer) {
        g_Camera.timer--;
        if (!g_Camera.timer) {
            g_Camera.timer = -1;
        }
    }
}

void Camera_Update(void)
{
    if (g_Camera.type == CAM_CINEMATIC) {
        Camera_LoadCutsceneFrame();
        return;
    }

    if (g_Camera.flags != NO_CHUNKY) {
        g_ChunkyFlag = true;
    }

    int32_t fixed_camera = g_Camera.item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    ITEM_INFO *item = fixed_camera ? g_Camera.item : g_LaraItem;

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (!fixed_camera) {
        y += bounds->max.y + ((bounds->min.y - bounds->max.y) * 3 >> 2);
    } else {
        y += (bounds->min.y + bounds->max.y) / 2;
    }

    if (g_Camera.item && !fixed_camera) {
        bounds = Item_GetBoundsAccurate(g_Camera.item);
        int16_t shift = Math_Sqrt(
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
        g_Camera.target.room_number = item->room_number;

        if (g_Camera.fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) >> 2;
            g_Camera.speed =
                g_Camera.type == CAM_LOOK ? LOOK_SPEED : COMBAT_SPEED;
        }

        g_Camera.fixed_camera = 0;

        if (g_Camera.type == CAM_LOOK) {
            Camera_Look(item);
        } else {
            Camera_Combat(item);
        }
    } else {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;

        if (g_Camera.flags == FOLLOW_CENTRE) {
            int16_t shift = (bounds->min.z + bounds->max.z) / 2;
            g_Camera.target.z += Math_Cos(item->rot.y) * shift >> W2V_SHIFT;
            g_Camera.target.x += Math_Sin(item->rot.y) * shift >> W2V_SHIFT;
        }

        g_Camera.target.room_number = item->room_number;

        if (g_Camera.fixed_camera != fixed_camera) {
            g_Camera.target.y = y;
            g_Camera.fixed_camera = 1;
            g_Camera.speed = 1;
        } else {
            g_Camera.target.y += (y - g_Camera.target.y) / 4;
            g_Camera.fixed_camera = 0;
        }

        FLOOR_INFO *floor = Room_GetFloor(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            &g_Camera.target.room_number);
        if (g_Camera.target.y > Room_GetHeight(
                floor, g_Camera.target.x, g_Camera.target.y,
                g_Camera.target.z)) {
            g_ChunkyFlag = false;
        }

        if (g_Camera.type == CAM_CHASE || g_Camera.flags == CHASE_OBJECT) {
            Camera_Chase(item);
        } else {
            Camera_Fixed();
        }
    }

    g_Camera.last = g_Camera.number;
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
    }

    if (g_Camera.type != CAM_HEAVY || g_Camera.timer == -1) {
        g_Camera.type = CAM_CHASE;
        g_Camera.number = NO_CAMERA;
        g_Camera.last_item = g_Camera.item;
        g_Camera.item = NULL;
        g_Camera.target_angle = g_Camera.additional_angle;
        g_Camera.target_elevation = g_Camera.additional_elevation;
        g_Camera.target_distance = WALL_L * 3 / 2;
        g_Camera.flags = 0;
    }

    g_ChunkyFlag = false;
}

static void Camera_EnsureEnvironment(void)
{
    if (g_Camera.pos.room_number == NO_ROOM) {
        return;
    }

    if (g_RoomInfo[g_Camera.pos.room_number].flags & RF_UNDERWATER) {
        Sound_Effect(SFX_UNDERWATER, NULL, SPM_ALWAYS);
        g_Camera.underwater = true;
    } else if (g_Camera.underwater) {
        Sound_StopEffect(SFX_UNDERWATER, NULL);
        g_Camera.underwater = false;
    }
}

void Camera_OffsetReset(void)
{
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

void Camera_UpdateCutscene(void)
{
    CINE_CAMERA *ref = &g_CineCamera[g_CineFrame];

    const int32_t c = Math_Cos(g_Camera.target_angle);
    const int32_t s = Math_Sin(g_Camera.target_angle);
    const XYZ_32 *const pos = &g_CinePosition.pos;
    g_Camera.target.x = pos->x + ((ref->tx * c + ref->tz * s) >> W2V_SHIFT);
    g_Camera.target.y = pos->y + ref->ty;
    g_Camera.target.z = pos->z + ((ref->tz * c - ref->tx * s) >> W2V_SHIFT);
    g_Camera.pos.x = pos->x + ((ref->cz * s + ref->cx * c) >> W2V_SHIFT);
    g_Camera.pos.y = pos->y + ref->cy;
    g_Camera.pos.z = pos->z + ((ref->cz * c - ref->cx * s) >> W2V_SHIFT);
    g_Camera.roll = ref->roll;
    g_Camera.shift = 0;

    Viewport_SetFOV(ref->fov);
}

void Camera_RefreshFromTrigger(int16_t type, int16_t *data)
{
    int16_t trigger;
    int16_t target_ok = 2;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_CAMERA:
            trigger = *data++;

            if (value == g_Camera.last) {
                g_Camera.number = value;

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
            break;

        case TO_TARGET:
            if (g_Camera.type != CAM_LOOK && g_Camera.type != CAM_COMBAT) {
                g_Camera.item = &g_Items[value];
            }
            break;
        }
    } while (!(trigger & END_BIT));

    if (g_Camera.item != NULL) {
        if (!target_ok
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item)) {
            g_Camera.item = NULL;
        }
    }
}

void Camera_MoveManual(void)
{
    int16_t camera_delta = (const int)PHD_90 / (const int)LOGIC_FPS
        * (double)m_ManualCameraMultiplier[g_Config.camera_speed];

    if (g_Input.camera_left) {
        Camera_OffsetAdditionalAngle(camera_delta);
    } else if (g_Input.camera_right) {
        Camera_OffsetAdditionalAngle(-camera_delta);
    }
    if (g_Input.camera_up) {
        Camera_OffsetAdditionalElevation(-camera_delta);
    } else if (g_Input.camera_down) {
        Camera_OffsetAdditionalElevation(camera_delta);
    }
    if (g_Input.camera_reset) {
        Camera_OffsetReset();
    }
}

void Camera_Apply(void)
{
    Camera_EnsureEnvironment();
    Matrix_LookAt(
        g_Camera.interp.result.pos.x,
        g_Camera.interp.result.pos.y + g_Camera.interp.result.shift,
        g_Camera.interp.result.pos.z, g_Camera.interp.result.target.x,
        g_Camera.interp.result.target.y, g_Camera.interp.result.target.z, 0);
}

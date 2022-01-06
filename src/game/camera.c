#include "game/camera.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/cinema.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <stddef.h>

void InitialiseCamera()
{
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

    g_Camera.number_frames = 1;
    g_Camera.type = CAM_CHASE;
    g_Camera.flags = 0;
    g_Camera.bounce = 0;
    g_Camera.number = NO_CAMERA;
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;

    CalculateCamera();
}

void MoveCamera(GAME_VECTOR *ideal, int32_t speed)
{
    g_Camera.pos.x += (ideal->x - g_Camera.pos.x) / speed;
    g_Camera.pos.z += (ideal->z - g_Camera.pos.z) / speed;
    g_Camera.pos.y += (ideal->y - g_Camera.pos.y) / speed;
    g_Camera.pos.room_number = ideal->room_number;

    g_ChunkyFlag = false;

    FLOOR_INFO *floor = GetFloor(
        g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
        &g_Camera.pos.room_number);
    int32_t height =
        GetHeight(floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
        - GROUND_SHIFT;

    if (g_Camera.pos.y >= height && ideal->y >= height) {
        LOS(&g_Camera.target, &g_Camera.pos);
        floor = GetFloor(
            g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z,
            &g_Camera.pos.room_number);
        height =
            GetHeight(floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
            - GROUND_SHIFT;
    }

    int32_t ceiling =
        GetCeiling(floor, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z)
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

    GetFloor(
        g_Camera.pos.x, g_Camera.pos.y + g_Camera.shift, g_Camera.pos.z,
        &g_Camera.pos.room_number);

    phd_LookAt(
        g_Camera.pos.x, g_Camera.pos.y + g_Camera.shift, g_Camera.pos.z,
        g_Camera.target.x, g_Camera.target.y, g_Camera.target.z, 0);

    g_Camera.actual_angle = phd_atan(
        g_Camera.target.z - g_Camera.pos.z, g_Camera.target.x - g_Camera.pos.x);
}

void ClipCamera(
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

void ShiftCamera(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    int32_t shift;

    int32_t TL_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    int32_t BL_square = SQUARE(target_x - left) + SQUARE(target_y - bottom);
    int32_t TR_square = SQUARE(target_x - right) + SQUARE(target_y - top);

    if (g_Camera.target_square < TL_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? -shift : shift);
    } else if (TL_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (g_Camera.target_square < BL_square) {
        *x = left;
        shift = g_Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? shift : -shift);
    } else if (BL_square > MIN_SQUARE) {
        *x = left;
        *y = bottom;
    } else if (g_Camera.target_square < TR_square) {
        shift = g_Camera.target_square - SQUARE(target_y - top);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *x = target_x + ((left < right) ? shift : -shift);
        *y = top;
    } else {
        *x = right;
        *y = top;
    }
}

int32_t BadPosition(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    if (y >= GetHeight(floor, x, y, z) || y <= GetCeiling(floor, x, y, z)) {
        return 1;
    }
    return 0;
}

void SmartShift(
    GAME_VECTOR *ideal,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom))
{
    LOS(&g_Camera.target, ideal);

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
    int32_t bad_left =
        BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_left) {
        camera_box = r->floor[x_floor - 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].left < left) {
            left = g_Boxes[camera_box].left;
        }
    }

    test = (ideal->z + WALL_L) & (~(WALL_L - 1));
    int32_t bad_right =
        BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_right) {
        camera_box = r->floor[x_floor + 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].right > right) {
            right = g_Boxes[camera_box].right;
        }
    }

    test = (ideal->x - WALL_L) | (WALL_L - 1);
    int32_t bad_top = BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_top) {
        camera_box = r->floor[x_floor + (y_floor - 1) * r->x_size].box;
        if (camera_box != NO_ITEM && g_Boxes[camera_box].top < top) {
            top = g_Boxes[camera_box].top;
        }
    }

    test = (ideal->x + WALL_L) & (~(WALL_L - 1));
    int32_t bad_bottom =
        BadPosition(test, ideal->y, ideal->z, ideal->room_number);
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
        GetFloor(ideal->x, ideal->y, ideal->z, &ideal->room_number);
    }
}

void ChaseCamera(ITEM_INFO *item)
{
    GAME_VECTOR ideal;

    g_Camera.target_elevation += item->pos.x_rot;
    if (g_Camera.target_elevation > MAX_ELEVATION) {
        g_Camera.target_elevation = MAX_ELEVATION;
    } else if (g_Camera.target_elevation < -MAX_ELEVATION) {
        g_Camera.target_elevation = -MAX_ELEVATION;
    }

    int32_t distance =
        g_Camera.target_distance * phd_cos(g_Camera.target_elevation)
        >> W2V_SHIFT;
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * phd_sin(g_Camera.target_elevation)
           >> W2V_SHIFT);

    g_Camera.target_square = SQUARE(distance);

    PHD_ANGLE angle = item->pos.y_rot + g_Camera.target_angle;
    ideal.x = g_Camera.target.x - (distance * phd_sin(angle) >> W2V_SHIFT);
    ideal.z = g_Camera.target.z - (distance * phd_cos(angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    SmartShift(&ideal, ShiftCamera);

    if (g_Camera.fixed_camera) {
        MoveCamera(&ideal, g_Camera.speed);
    } else {
        MoveCamera(&ideal, CHASE_SPEED);
    }
}

int32_t ShiftClamp(GAME_VECTOR *pos, int32_t clamp)
{
    int32_t x = pos->x;
    int32_t y = pos->y;
    int32_t z = pos->z;

    FLOOR_INFO *floor = GetFloor(x, y, z, &pos->room_number);

    BOX_INFO *box = &g_Boxes[floor->box];
    if (z < box->left + clamp
        && BadPosition(x, y, z - clamp, pos->room_number)) {
        pos->z = box->left + clamp;
    } else if (
        z > box->right - clamp
        && BadPosition(x, y, z + clamp, pos->room_number)) {
        pos->z = box->right - clamp;
    }

    if (x < box->top + clamp
        && BadPosition(x - clamp, y, z, pos->room_number)) {
        pos->x = box->top + clamp;
    } else if (
        x > box->bottom - clamp
        && BadPosition(x + clamp, y, z, pos->room_number)) {
        pos->x = box->bottom - clamp;
    }

    int32_t height = GetHeight(floor, x, y, z) - clamp;
    int32_t ceiling = GetCeiling(floor, x, y, z) + clamp;

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

void CombatCamera(ITEM_INFO *item)
{
    GAME_VECTOR ideal;

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    if (g_Lara.target) {
        g_Camera.target_angle = item->pos.y_rot + g_Lara.target_angles[0];
        g_Camera.target_elevation = item->pos.x_rot + g_Lara.target_angles[1];
    } else {
        g_Camera.target_angle =
            item->pos.y_rot + g_Lara.torso_y_rot + g_Lara.head_y_rot;
        g_Camera.target_elevation =
            item->pos.x_rot + g_Lara.torso_x_rot + g_Lara.head_x_rot;
    }

    g_Camera.target_distance = COMBAT_DISTANCE;

    int32_t distance =
        g_Camera.target_distance * phd_cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    ideal.x = g_Camera.target.x
        - (distance * phd_sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * phd_sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * phd_cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    SmartShift(&ideal, ShiftCamera);
    MoveCamera(&ideal, g_Camera.speed);
}

void LookCamera(ITEM_INFO *item)
{
    GAME_VECTOR old;
    GAME_VECTOR ideal;

    old.z = g_Camera.target.z;
    old.x = g_Camera.target.x;

    g_Camera.target.z = item->pos.z;
    g_Camera.target.x = item->pos.x;

    g_Camera.target_angle =
        item->pos.y_rot + g_Lara.torso_y_rot + g_Lara.head_y_rot;
    g_Camera.target_elevation =
        item->pos.x_rot + g_Lara.torso_x_rot + g_Lara.head_x_rot;
    g_Camera.target_distance = WALL_L * 3 / 2;

    int32_t distance =
        g_Camera.target_distance * phd_cos(g_Camera.target_elevation)
        >> W2V_SHIFT;

    g_Camera.shift =
        -STEP_L * 2 * phd_sin(g_Camera.target_elevation) >> W2V_SHIFT;
    g_Camera.target.z += g_Camera.shift * phd_cos(item->pos.y_rot) >> W2V_SHIFT;
    g_Camera.target.x += g_Camera.shift * phd_sin(item->pos.y_rot) >> W2V_SHIFT;

    if (BadPosition(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            g_Camera.target.room_number)) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;
    }

    g_Camera.target.y += ShiftClamp(&g_Camera.target, STEP_L + 50);

    ideal.x = g_Camera.target.x
        - (distance * phd_sin(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.y = g_Camera.target.y
        + (g_Camera.target_distance * phd_sin(g_Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = g_Camera.target.z
        - (distance * phd_cos(g_Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = g_Camera.pos.room_number;

    SmartShift(&ideal, ClipCamera);

    g_Camera.target.z = old.z + (g_Camera.target.z - old.z) / g_Camera.speed;
    g_Camera.target.x = old.x + (g_Camera.target.x - old.x) / g_Camera.speed;

    MoveCamera(&ideal, g_Camera.speed);
}

void FixedCamera()
{
    GAME_VECTOR ideal;
    OBJECT_VECTOR *fixed;

    fixed = &g_Camera.fixed[g_Camera.number];
    ideal.x = fixed->x;
    ideal.y = fixed->y;
    ideal.z = fixed->z;
    ideal.room_number = fixed->data;

    if (!LOS(&g_Camera.target, &ideal)) {
        ShiftClamp(&ideal, STEP_L);
    }

    g_Camera.fixed_camera = 1;

    MoveCamera(&ideal, g_Camera.speed);

    if (g_Camera.timer) {
        g_Camera.timer--;
        if (!g_Camera.timer) {
            g_Camera.timer = -1;
        }
    }
}

void CalculateCamera()
{
    if (g_RoomInfo[g_Camera.pos.room_number].flags & RF_UNDERWATER) {
        Sound_Effect(SFX_UNDERWATER, NULL, SPM_ALWAYS);
        if (!g_Camera.underwater) {
            g_Camera.underwater = 1;
        }
    } else if (g_Camera.underwater) {
        Sound_StopEffect(SFX_UNDERWATER, NULL);
        g_Camera.underwater = 0;
    }

    if (g_Camera.type == CAM_CINEMATIC) {
        InGameCinematicCamera();
        return;
    }

    if (g_Camera.flags != NO_CHUNKY) {
        g_ChunkyFlag = true;
    }

    int32_t fixed_camera = g_Camera.item
        && (g_Camera.type == CAM_FIXED || g_Camera.type == CAM_HEAVY);
    ITEM_INFO *item = fixed_camera ? g_Camera.item : g_LaraItem;

    int16_t *bounds = GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (!fixed_camera) {
        y += bounds[FRAME_BOUND_MAX_Y]
            + ((bounds[FRAME_BOUND_MIN_Y] - bounds[FRAME_BOUND_MAX_Y]) * 3
               >> 2);
    } else {
        y += (bounds[FRAME_BOUND_MIN_Y] + bounds[FRAME_BOUND_MAX_Y]) / 2;
    }

    if (g_Camera.item && !fixed_camera) {
        bounds = GetBoundsAccurate(g_Camera.item);
        int16_t shift = phd_sqrt(
            SQUARE(g_Camera.item->pos.z - item->pos.z)
            + SQUARE(g_Camera.item->pos.x - item->pos.x));
        int16_t angle = phd_atan(
                            g_Camera.item->pos.z - item->pos.z,
                            g_Camera.item->pos.x - item->pos.x)
            - item->pos.y_rot;
        int16_t tilt = phd_atan(
            shift,
            y
                - (g_Camera.item->pos.y
                   + (bounds[FRAME_BOUND_MIN_Y] + bounds[FRAME_BOUND_MAX_Y])
                       / 2));
        angle >>= 1;
        tilt >>= 1;

        if (angle > -MAX_HEAD_ROTATION && angle < MAX_HEAD_ROTATION
            && tilt > MIN_HEAD_TILT_CAM && tilt < MAX_HEAD_TILT_CAM) {
            int16_t change = angle - g_Lara.head_y_rot;
            if (change > HEAD_TURN) {
                g_Lara.head_y_rot += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_y_rot -= HEAD_TURN;
            } else {
                g_Lara.head_y_rot += change;
            }

            change = tilt - g_Lara.head_x_rot;
            if (change > HEAD_TURN) {
                g_Lara.head_x_rot += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                g_Lara.head_x_rot -= HEAD_TURN;
            } else {
                g_Lara.head_x_rot += change;
            }

            g_Lara.torso_y_rot = g_Lara.head_y_rot;
            g_Lara.torso_x_rot = g_Lara.head_x_rot;

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
            LookCamera(item);
        } else {
            CombatCamera(item);
        }
    } else {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.z = item->pos.z;

        if (g_Camera.flags == FOLLOW_CENTRE) {
            int16_t shift =
                (bounds[FRAME_BOUND_MIN_Z] + bounds[FRAME_BOUND_MAX_Z]) / 2;
            g_Camera.target.z += phd_cos(item->pos.y_rot) * shift >> W2V_SHIFT;
            g_Camera.target.x += phd_sin(item->pos.y_rot) * shift >> W2V_SHIFT;
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

        FLOOR_INFO *floor = GetFloor(
            g_Camera.target.x, g_Camera.target.y, g_Camera.target.z,
            &g_Camera.target.room_number);
        if (g_Camera.target.y > GetHeight(
                floor, g_Camera.target.x, g_Camera.target.y,
                g_Camera.target.z)) {
            g_ChunkyFlag = false;
        }

        if (g_Camera.type == CAM_CHASE || g_Camera.flags == CHASE_OBJECT) {
            ChaseCamera(item);
        } else {
            FixedCamera();
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

void CameraOffsetAdditionalAngle(int16_t delta)
{
    g_Camera.additional_angle += delta;
}

void CameraOffsetAdditionalElevation(int16_t delta)
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

void CameraOffsetReset()
{
    g_Camera.additional_angle = 0;
    g_Camera.additional_elevation = 0;
}

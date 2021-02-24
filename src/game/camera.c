#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/camera.h"
#include "game/const.h"
#include "game/control.h"
#include "game/game.h"
#include "game/misc.h"
#include "game/vars.h"
#include "util.h"

void InitialiseCamera()
{
    Camera.shift = LaraItem->pos.y - WALL_L;

    Camera.target.x = LaraItem->pos.x;
    Camera.target.y = Camera.shift;
    Camera.target.z = LaraItem->pos.z;
    Camera.target.room_number = LaraItem->room_number;

    Camera.pos.x = Camera.target.x;
    Camera.pos.y = Camera.target.y;
    Camera.pos.z = Camera.target.z - 100;
    Camera.pos.room_number = Camera.target.room_number;

    Camera.target_distance = WALL_L * 3 / 2;
    Camera.item = NULL;

    Camera.number_frames = 1;
    Camera.type = CAM_CHASE;
    Camera.flags = 0;
    Camera.bounce = 0;
    Camera.number = NO_CAMERA;

    CalculateCamera();
}

void MoveCamera(GAME_VECTOR* ideal, int32_t speed)
{
    Camera.pos.x += (ideal->x - Camera.pos.x) / speed;
    Camera.pos.z += (ideal->z - Camera.pos.z) / speed;
    Camera.pos.y += (ideal->y - Camera.pos.y) / speed;
    Camera.pos.room_number = ideal->room_number;

    ChunkyFlag = 0;

    FLOOR_INFO* floor = GetFloor(
        Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
    int32_t height = GetHeight(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
        - GROUND_SHIFT;

    if (Camera.pos.y >= height && ideal->y >= height) {
        LOS(&Camera.target, &Camera.pos);
        floor = GetFloor(
            Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
        height = GetHeight(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
            - GROUND_SHIFT;
    }

    int32_t ceiling =
        GetCeiling(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
        + GROUND_SHIFT;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (Camera.bounce) {
        if (Camera.bounce > 0) {
            Camera.pos.y += Camera.bounce;
            Camera.target.y += Camera.bounce;
            Camera.bounce = 0;
        } else {
            int32_t shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.x += shake;
            Camera.target.y += shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.y += shake;
            Camera.target.y += shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.z += shake;
            Camera.target.z += shake;
            Camera.bounce += 5;
        }
    }

    if (Camera.pos.y > height) {
        Camera.shift = height - Camera.pos.y;
    } else if (Camera.pos.y < ceiling) {
        Camera.shift = ceiling - Camera.pos.y;
    } else {
        Camera.shift = 0;
    }

    GetFloor(
        Camera.pos.x, Camera.pos.y + Camera.shift, Camera.pos.z,
        &Camera.pos.room_number);

    phd_LookAt(
        Camera.pos.x, Camera.pos.y + Camera.shift, Camera.pos.z,
        Camera.target.x, Camera.target.y, Camera.target.z, 0);

    Camera.actual_angle = phd_atan(
        Camera.target.z - Camera.pos.z, Camera.target.x - Camera.pos.x);
}

void ClipCamera(
    int32_t* x, int32_t* y, int32_t target_x, int32_t target_y, int32_t left,
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
    int32_t* x, int32_t* y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    int32_t shift;

    int32_t TL_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    int32_t BL_square = SQUARE(target_x - left) + SQUARE(target_y - bottom);
    int32_t TR_square = SQUARE(target_x - right) + SQUARE(target_y - top);

    if (Camera.target_square < TL_square) {
        *x = left;
        shift = Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? -shift : shift);
    } else if (TL_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (Camera.target_square < BL_square) {
        *x = left;
        shift = Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? shift : -shift);
    } else if (BL_square > MIN_SQUARE) {
        *x = left;
        *y = bottom;
    } else if (Camera.target_square < TR_square) {
        shift = Camera.target_square - SQUARE(target_y - top);
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
    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
    if (y >= GetHeight(floor, x, y, z) || y <= GetCeiling(floor, x, y, z)) {
        return 1;
    }
    return 0;
}

void SmartShift(
    GAME_VECTOR* ideal,
    void (*shift)(
        int32_t* x, int32_t* y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom))
{
    LOS(&Camera.target, ideal);

    ROOM_INFO* r = &RoomInfo[Camera.target.room_number];
    int32_t x_floor = (Camera.target.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (Camera.target.x - r->x) >> WALL_SHIFT;

    int16_t item_box = r->floor[x_floor + y_floor * r->x_size].box;
    BOX_INFO* box = &Boxes[item_box];

    r = &RoomInfo[ideal->room_number];
    x_floor = (ideal->z - r->z) >> WALL_SHIFT;
    y_floor = (ideal->x - r->x) >> WALL_SHIFT;

    int16_t camera_box = r->floor[x_floor + y_floor * r->x_size].box;
    if (camera_box != NO_BOX
        && (ideal->z < box->left || ideal->z > box->right || ideal->x < box->top
            || ideal->x > box->bottom)) {
        box = &Boxes[camera_box];
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
        if (camera_box != NO_ITEM && Boxes[camera_box].left < left) {
            left = Boxes[camera_box].left;
        }
    }

    test = (ideal->z + WALL_L) & (~(WALL_L - 1));
    int32_t bad_right =
        BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_right) {
        camera_box = r->floor[x_floor + 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].right > right) {
            right = Boxes[camera_box].right;
        }
    }

    test = (ideal->x - WALL_L) | (WALL_L - 1);
    int32_t bad_top = BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_top) {
        camera_box = r->floor[x_floor + (y_floor - 1) * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].top < top) {
            top = Boxes[camera_box].top;
        }
    }

    test = (ideal->x + WALL_L) & (~(WALL_L - 1));
    int32_t bad_bottom =
        BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_bottom) {
        camera_box = r->floor[x_floor + (y_floor + 1) * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].bottom > bottom) {
            bottom = Boxes[camera_box].bottom;
        }
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    int32_t noclip = 1;
    if (ideal->z < left && bad_left) {
        noclip = 0;
        if (ideal->x < Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, left,
                top, right, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, left,
                bottom, right, top);
        }
    } else if (ideal->z > right && bad_right) {
        noclip = 0;
        if (ideal->x < Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, right,
                top, left, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, right,
                bottom, left, top);
        }
    }

    if (noclip) {
        if (ideal->x < top && bad_top) {
            noclip = 0;
            if (ideal->z < Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z, top,
                    left, bottom, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z, top,
                    right, bottom, left);
            }
        } else if (ideal->x > bottom && bad_bottom) {
            noclip = 0;
            if (ideal->z < Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z,
                    bottom, left, top, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z,
                    bottom, right, top, left);
            }
        }
    }

    if (!noclip) {
        GetFloor(ideal->x, ideal->y, ideal->z, &ideal->room_number);
    }
}

void ChaseCamera(ITEM_INFO* item)
{
    GAME_VECTOR ideal;

    Camera.target_elevation += item->pos.x_rot;
    if (Camera.target_elevation > MAX_ELEVATION) {
        Camera.target_elevation = MAX_ELEVATION;
    } else if (Camera.target_elevation < -MAX_ELEVATION) {
        Camera.target_elevation = -MAX_ELEVATION;
    }

    int32_t distance =
        Camera.target_distance * phd_cos(Camera.target_elevation) >> W2V_SHIFT;
    ideal.y = Camera.target.y
        + (Camera.target_distance * phd_sin(Camera.target_elevation)
           >> W2V_SHIFT);

    Camera.target_square = SQUARE(distance);

    PHD_ANGLE angle = item->pos.y_rot + Camera.target_angle;
    ideal.x = Camera.target.x - (distance * phd_sin(angle) >> W2V_SHIFT);
    ideal.z = Camera.target.z - (distance * phd_cos(angle) >> W2V_SHIFT);
    ideal.room_number = Camera.pos.room_number;

    SmartShift(&ideal, ShiftCamera);

    if (Camera.fixed_camera) {
        MoveCamera(&ideal, Camera.speed);
    } else {
        MoveCamera(&ideal, CHASE_SPEED);
    }
}

int32_t ShiftClamp(GAME_VECTOR* pos, int32_t clamp)
{
    int32_t x = pos->x;
    int32_t y = pos->y;
    int32_t z = pos->z;

    FLOOR_INFO* floor = GetFloor(x, y, z, &pos->room_number);

    BOX_INFO* box = &Boxes[floor->box];
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

void T1MInjectGameCamera()
{
    INJECT(0x0040F920, InitialiseCamera);
    INJECT(0x0040F9B0, MoveCamera);
    INJECT(0x0040FCA0, ClipCamera);
    INJECT(0x0040FD40, ShiftCamera);
    INJECT(0x0040FEA0, SmartShift);
    INJECT(0x00410410, ChaseCamera);
    INJECT(0x00410530, ShiftClamp);
}

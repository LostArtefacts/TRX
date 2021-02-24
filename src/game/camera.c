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

void T1MInjectGameCamera()
{
    INJECT(0x0040F920, InitialiseCamera);
    INJECT(0x0040F9B0, MoveCamera);
    INJECT(0x0040FCA0, ClipCamera);
    INJECT(0x0040FD40, ShiftCamera);
}

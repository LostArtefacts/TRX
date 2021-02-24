#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/camera.h"
#include "game/const.h"
#include "game/control.h"
#include "game/game.h"
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

void T1MInjectGameCamera()
{
    INJECT(0x0040F920, InitialiseCamera);
    INJECT(0x0040F9B0, MoveCamera);
}

#include "game/camera.h"
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

void T1MInjectGameCamera()
{
    INJECT(0x0040F920, InitialiseCamera);
}

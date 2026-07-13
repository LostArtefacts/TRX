// A camera standing still in a known place. The real one is driven by the game
// loop; the surface only ever reads it, so the whole engine underneath is a
// struct and two counters.

#include "fake_engine_camera.h"

#include <trx/game/camera/types.h>
#include <trx/game/rooms/const.h>

CAMERA_INFO g_Camera;
FAKE_CAMERA_CALLS g_FakeCameraCalls;

void Camera_ResetPosition(void)
{
    g_FakeCameraCalls.reset++;
}

void FakeCamera_Reset(void)
{
    g_Camera = (CAMERA_INFO) {};
    g_Camera.pos.x = 1024;
    g_Camera.pos.y = 2048;
    g_Camera.pos.z = 3072;
    // The engine counts rooms from 0 and Lua from 1.
    g_Camera.pos.room_num = FAKE_CAMERA_ROOM - 1;
    g_Camera.target.x = 4096;
    g_Camera.target.y = 5120;
    g_Camera.target.z = 6144;
    g_Camera.target.room_num = FAKE_CAMERA_TARGET_ROOM - 1;
    g_FakeCameraCalls = (FAKE_CAMERA_CALLS) {};
}

void FakeCamera_SetNoRoom(void)
{
    g_Camera.pos.room_num = NO_ROOM;
    g_Camera.target.room_num = NO_ROOM;
}

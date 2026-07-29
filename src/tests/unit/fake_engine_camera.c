// A camera standing still in a known place. The real one is driven by the game
// loop; the surface only ever reads it, so the whole engine underneath is a
// struct and two counters.

#include "fake_engine_camera.h"

#include <trx/game/camera/types.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/rooms/const.h>

CAMERA_INFO g_Camera;
FAKE_CAMERA_CALLS g_FakeCameraCalls;
static bool m_FlybyActive;

void Camera_ResetPosition(void)
{
    g_FakeCameraCalls.reset++;
}

bool FlybyMode_IsActive(void)
{
    return m_FlybyActive;
}

bool FlybyMode_Activate(const int32_t sequence_idx, const bool one_shot)
{
    g_FakeCameraCalls.play_flyby++;
    g_FakeCameraCalls.last_flyby_sequence = sequence_idx;
    // A sequence already holding the camera is what the real one turns away;
    // the fake answers the same way so a test can see the refusal.
    const bool was_active = m_FlybyActive;
    m_FlybyActive = true;
    return !was_active;
}

bool FlybyMode_Cancel(void)
{
    g_FakeCameraCalls.cancel_flyby++;
    m_FlybyActive = false;
    return true;
}

void FakeCamera_SetFlybyActive(const bool active)
{
    m_FlybyActive = active;
}

void FakeCamera_Reset(void)
{
    g_Camera = (CAMERA_INFO) {};
    g_Camera.pos.x = 1024;
    g_Camera.pos.y = 2048;
    g_Camera.pos.z = 3072;
    // The engine and Lua both count rooms from 0.
    g_Camera.pos.room_num = FAKE_CAMERA_ROOM;
    g_Camera.target.x = 4096;
    g_Camera.target.y = 5120;
    g_Camera.target.z = 6144;
    g_Camera.target.room_num = FAKE_CAMERA_TARGET_ROOM;
    g_FakeCameraCalls = (FAKE_CAMERA_CALLS) {};
    m_FlybyActive = false;
}

void FakeCamera_SetNoRoom(void)
{
    g_Camera.pos.room_num = NO_ROOM;
    g_Camera.target.room_num = NO_ROOM;
}

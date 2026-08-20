// A camera standing still in a known place. The real one is driven by the game
// loop; the surface only ever reads it, so the whole engine underneath is a
// struct and two counters.

#include <fakes/camera.h>

#include <harness/fake_calls.h>

#include <trx/game/camera/types.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/rooms/const.h>

CAMERA_INFO g_Camera;
static bool m_FlybyActive;

static void M_Reset(void)
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
    m_FlybyActive = false;
}

void Camera_ResetPosition(void)
{
    FAKE_RECORD("reset");
}

bool FlybyMode_IsActive(void)
{
    return m_FlybyActive;
}

bool FlybyMode_Activate(const int32_t sequence_idx, const bool one_shot)
{
    FAKE_RECORD("play_flyby", FV(sequence_idx));
    // A sequence already holding the camera is what the real one turns away;
    // the fake answers the same way so a test can see the refusal.
    const bool was_active = m_FlybyActive;
    m_FlybyActive = true;
    return !was_active;
}

bool FlybyMode_Cancel(const bool force)
{
    FAKE_RECORD("cancel_flyby");
    m_FlybyActive = false;
    return true;
}

void FakeCamera_SetFlybyActive(const bool active)
{
    m_FlybyActive = active;
}

FAKE_ON_RESET(M_Reset)

void FakeCamera_SetNoRoom(void)
{
    g_Camera.pos.room_num = NO_ROOM;
    g_Camera.target.room_num = NO_ROOM;
}

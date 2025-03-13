#include "game/camera/photo_mode.h"

#include "game/camera/common.h"
#include "game/camera/const.h"
#include "game/camera/enum.h"
#include "game/camera/vars.h"
#include "game/input.h"
#include "game/math.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/viewport.h"
#include "utils.h"

#if TR_VERSION == 1
// TODO: remove this call when consolidating the viewport API
extern void Output_ApplyFOV(void);
#endif

#define MIN_PHOTO_FOV 10
#define MAX_PHOTO_FOV 150
#define PHOTO_ROT_SHIFT (DEG_1 * 4)
#define PHOTO_MAX_PITCH_ROLL (DEG_90 - DEG_1)
#define PHOTO_MAX_SPEED 100

#define M_CosMul(a, b) ((Math_Cos(a) * (b)) >> W2V_SHIFT)
#define M_SinMul(a, b) ((Math_Sin(a) * (b)) >> W2V_SHIFT)

static int32_t m_PhotoSpeed = 0;
static int32_t m_OldFOV;
static int32_t m_CurrentFOV;
static CAMERA_INFO m_OldCamera = {};
static BOUNDS_32 m_WorldBounds = {};

static int32_t M_GetShiftSpeed(int32_t val);
static int32_t M_GetRotSpeed(void);
static void M_ShiftCamera(int32_t dx, int32_t dy, int32_t dz);
static void M_ApplyRotation(
    int32_t d_yaw, int32_t d_pitch, int32_t d_roll, bool respect_roll);
static void M_RotateCamera(int32_t d_yaw, int32_t d_pitch, int32_t d_roll);
static void M_RotateTarget(int32_t d_yaw, int32_t d_pitch, int32_t d_roll);
static void M_ClampCameraPos(void);
static void M_UpdateCameraRooms(void);
static bool M_HandleShiftInputs(void);
static bool M_HandleRotationInputs(void);
static bool M_HandleTargetRotationInputs(void);
static bool M_HandleFOVInputs();
static void M_UpdatePhotoMode(void);

static void M_ResetCamera(void)
{
    g_Camera = m_OldCamera;
#if TR_VERSION == 1
    Viewport_SetFOV(m_OldFOV);
#elif TR_VERSION == 2
    Viewport_AlterFOV(m_OldFOV);
#endif
    m_CurrentFOV = m_OldFOV / DEG_1;
}

static int32_t M_GetShiftSpeed(const int32_t val)
{
    return val * m_PhotoSpeed / (float)PHOTO_MAX_SPEED;
}

static int32_t M_GetRotSpeed(void)
{
    return MAX(DEG_1, M_GetShiftSpeed(PHOTO_ROT_SHIFT));
}

static XYZ_32 M_GetShift(const int32_t dx, const int32_t dy, const int32_t dz)
{
    const int16_t yaw = g_Camera.target_angle;
    const int16_t pitch = g_Camera.target_elevation;
    const int16_t roll = g_Camera.roll;

    const int32_t dx_r = M_CosMul(roll, dx) - M_SinMul(roll, dy);
    const int32_t dy_r = M_SinMul(roll, dx) + M_CosMul(roll, dy);
    const int32_t dz_r = dz; // unchanged if roll is around Z

    const int32_t dy_p = M_CosMul(pitch, dy_r) - M_SinMul(pitch, dz_r);
    const int32_t dz_p = M_SinMul(pitch, dy_r) + M_CosMul(pitch, dz_r);
    const int32_t dx_p = dx_r; // unchanged if pitch is around X

    const int32_t dx_y = M_CosMul(yaw, dx_p) + M_SinMul(yaw, dz_p);
    const int32_t dz_y = -M_SinMul(yaw, dx_p) + M_CosMul(yaw, dz_p);
    const int32_t dy_y = dy_p; // unchanged if yaw is around Y

    return (XYZ_32) { dx_y, dy_y, dz_y };
}

static void M_ShiftCamera(int32_t dx, int32_t dy, int32_t dz)
{
    const int32_t distance = M_GetShiftSpeed((WALL_L * 5.0) / LOGIC_FPS);
    dx *= distance;
    dy *= distance;
    dz *= distance;

    const XYZ_32 shift = M_GetShift(dx, dy, dz);
    g_Camera.pos.x += shift.x;
    g_Camera.pos.y += shift.y;
    g_Camera.pos.z += shift.z;
    g_Camera.target.x += shift.x;
    g_Camera.target.y += shift.y;
    g_Camera.target.z += shift.z;
}

static void M_ApplyRotation(
    const int32_t d_yaw, const int32_t d_pitch, const int32_t d_roll,
    const bool respect_roll)
{
    int32_t yaw = g_Camera.target_angle;
    int32_t pitch = g_Camera.target_elevation;
    int32_t roll = g_Camera.roll;

    // rotate with respect to current upright axis
    if (respect_roll) {
        yaw += M_CosMul(roll, d_yaw) + M_SinMul(roll, d_pitch);
        pitch += M_CosMul(roll, d_pitch) - M_SinMul(roll, d_yaw);
    } else {
        yaw += d_yaw;
        pitch += d_pitch;
    }
    roll += d_roll;

    // handle pivoting
    if (pitch >= DEG_90 || pitch <= -DEG_90) {
        roll += DEG_180;
        yaw += DEG_180;
        pitch = g_Camera.target_elevation;
    }

    g_Camera.target_angle = yaw;
    g_Camera.target_elevation = pitch;
    g_Camera.roll = roll;
}

static void M_RotateCamera(
    const int32_t d_yaw, const int32_t d_pitch, const int32_t d_roll)
{
    M_ApplyRotation(d_yaw, d_pitch, d_roll, true);
    const XYZ_32 shift = M_GetShift(0, 0, g_Camera.target_distance);
    g_Camera.target.x = g_Camera.pos.x + shift.x;
    g_Camera.target.y = g_Camera.pos.y + shift.y;
    g_Camera.target.z = g_Camera.pos.z + shift.z;
}

static void M_RotateTarget(
    const int32_t d_yaw, const int32_t d_pitch, const int32_t d_roll)
{
    M_ApplyRotation(d_yaw, d_pitch, d_roll, false);
    const XYZ_32 shift = M_GetShift(0, 0, g_Camera.target_distance);
    g_Camera.pos.x = g_Camera.target.x - shift.x;
    g_Camera.pos.y = g_Camera.target.y - shift.y;
    g_Camera.pos.z = g_Camera.target.z - shift.z;
}

static void M_ClampCameraPos(void)
{
    // While the camera is free, we want to clamp to within overall world bounds
    // to help counteract getting lost in the void.
    const GAME_VECTOR prev_cam_pos = g_Camera.pos;
    CLAMP(g_Camera.pos.x, m_WorldBounds.min.x, m_WorldBounds.max.x);
    CLAMP(g_Camera.pos.y, m_WorldBounds.min.y, m_WorldBounds.max.y);
    CLAMP(g_Camera.pos.z, m_WorldBounds.min.z, m_WorldBounds.max.z);

    g_Camera.target.x += (g_Camera.pos.x - prev_cam_pos.x);
    g_Camera.target.y += (g_Camera.pos.y - prev_cam_pos.y);
    g_Camera.target.z += (g_Camera.pos.z - prev_cam_pos.z);
}

static void M_UpdateCameraRooms(void)
{
    const int16_t pos_room_num =
        Room_GetIndexFromPos(g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    const int16_t tar_room_num = Room_GetIndexFromPos(
        g_Camera.target.x, g_Camera.target.y, g_Camera.target.z);

    if (pos_room_num != NO_ROOM) {
        g_Camera.pos.room_num = pos_room_num;
        if (tar_room_num == NO_ROOM) {
            g_Camera.target.room_num = pos_room_num;
        }
    }
    if (tar_room_num != NO_ROOM) {
        g_Camera.target.room_num = tar_room_num;
        if (pos_room_num == NO_ROOM) {
            g_Camera.pos.room_num = tar_room_num;
        }
    }
}

static bool M_HandleShiftInputs(void)
{
    bool result = false;

    if (g_Input.camera_left) {
        M_ShiftCamera(-1, 0, 0);
        result = true;
    } else if (g_Input.camera_right) {
        M_ShiftCamera(1, 0, 0);
        result = true;
    }

    if (g_Input.camera_forward) {
        M_ShiftCamera(0, 0, 1);
        result = true;
    } else if (g_Input.camera_back) {
        M_ShiftCamera(0, 0, -1);
        result = true;
    }

    if (g_Input.camera_up) {
        M_ShiftCamera(0, -1, 0);
        result = true;
    } else if (g_Input.camera_down) {
        M_ShiftCamera(0, 1, 0);
        result = true;
    }

    return result;
}

static bool M_HandleRotationInputs(void)
{
    bool result = false;

    if (g_Input.forward) {
        M_RotateCamera(0, -M_GetRotSpeed(), 0);
        result = true;
    } else if (g_Input.back) {
        M_RotateCamera(0, M_GetRotSpeed(), 0);
        result = true;
    }

    if (g_Input.left) {
        M_RotateCamera(-M_GetRotSpeed(), 0, 0);
        result = true;
    } else if (g_Input.right) {
        M_RotateCamera(M_GetRotSpeed(), 0, 0);
        result = true;
    }

    if (g_Input.step_left) {
        M_RotateCamera(0, 0, -M_GetRotSpeed());
        result = true;
    } else if (g_Input.step_right) {
        M_RotateCamera(0, 0, M_GetRotSpeed());
        result = true;
    }

    return result;
}

static bool M_HandleTargetRotationInputs(void)
{
    bool result = false;
    if (g_InputDB.roll) {
        M_RotateTarget(-DEG_90, 0, 0);
        result = true;
    }
    return result;
}

static bool M_HandleFOVInputs(void)
{
    if (!g_Input.draw) {
        return false;
    }

    if (g_Input.slow) {
        m_CurrentFOV--;
    } else {
        m_CurrentFOV++;
    }
    CLAMP(m_CurrentFOV, MIN_PHOTO_FOV, MAX_PHOTO_FOV);
#if TR_VERSION == 1
    Viewport_SetFOV(m_CurrentFOV * DEG_1);
    Output_ApplyFOV();
#elif TR_VERSION == 2
    Viewport_AlterFOV(m_CurrentFOV * DEG_1);
#endif
    return true;
}

void Camera_EnterPhotoMode(void)
{
    int16_t angles[2];
    Math_GetVectorAngles(
        g_Camera.target.x - g_Camera.pos.x, g_Camera.target.y - g_Camera.pos.y,
        g_Camera.target.z - g_Camera.pos.z, angles);
    g_Camera.target_angle = angles[0];
    g_Camera.target_elevation = angles[1];
    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.target_square = SQUARE(g_Camera.target_distance);

#if TR_VERSION == 1
    m_OldFOV = Viewport_GetFOV();
#elif TR_VERSION == 2
    m_OldFOV = Viewport_GetFOV(true);
#endif
    m_CurrentFOV = m_OldFOV / DEG_1;
    m_OldCamera = g_Camera;
    g_Camera.type = CAM_PHOTO_MODE;
    m_WorldBounds = Room_GetWorldBounds();
    M_UpdateCameraRooms();
}

void Camera_ExitPhotoMode(void)
{
#if TR_VERSION == 1
    Viewport_SetFOV(m_OldFOV);
#elif TR_VERSION == 2
    Viewport_AlterFOV(m_OldFOV);
#endif
    M_ResetCamera();
}

void Camera_UpdatePhotoMode(void)
{
    if (g_InputDB.camera_reset) {
        M_ResetCamera();
        g_Camera.type = CAM_PHOTO_MODE;
    }

    M_HandleFOVInputs();

    bool changed = false;
    changed |= M_HandleShiftInputs();
    changed |= M_HandleRotationInputs();

    if (changed) {
        m_PhotoSpeed++;
        CLAMPG(m_PhotoSpeed, PHOTO_MAX_SPEED);
    } else {
        m_PhotoSpeed = 0;
    }

    changed |= M_HandleTargetRotationInputs();

    if (changed) {
        M_ClampCameraPos();
        M_UpdateCameraRooms();
    }
}

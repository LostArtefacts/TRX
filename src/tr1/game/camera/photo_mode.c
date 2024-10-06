#include "game/camera/photo_mode.h"

#include "game/camera/common.h"
#include "game/input.h"
#include "game/output.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/math_misc.h"

#include <libtrx/utils.h>

#define MIN_PHOTO_FOV 10
#define MAX_PHOTO_FOV 150
#define PHOTO_ROT_SHIFT (PHD_DEGREE * 4)
#define PHOTO_MAX_PITCH_ROLL (PHD_90 - PHD_DEGREE)
#define PHOTO_MAX_SPEED 100

#define SHIFT_X(distance, elevation, angle)                                    \
    (((distance * Math_Cos(elevation)) >> W2V_SHIFT) * Math_Sin(angle)         \
     >> W2V_SHIFT)
#define SHIFT_Z(distance, elevation, angle)                                    \
    (((distance * Math_Cos(elevation)) >> W2V_SHIFT) * Math_Cos(angle)         \
     >> W2V_SHIFT)
#define SHIFT_Y(distance, elevation)                                           \
    ((distance * Math_Sin(elevation) >> W2V_SHIFT))

#define SHIFT_POS(a, b)                                                        \
    do {                                                                       \
        a.x += b.x;                                                            \
        a.y += b.y;                                                            \
        a.z += b.z;                                                            \
    } while (false)

static int32_t m_PhotoSpeed = 0;
static int32_t m_OldFOV;
static int32_t m_CurrentFOV;
static CAMERA_INFO m_OldCamera = { 0 };
static BOUNDS_32 m_WorldBounds = { 0 };

static int32_t M_GetShiftSpeed(int32_t val);
static int32_t M_GetRotSpeed(void);
static void M_ShiftCamera(int16_t angle, int16_t elevation, int32_t dy);
static void M_RotateCamera(int16_t angle, int16_t elevation);
static void M_RotateTarget(int16_t angle);
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
    Viewport_SetFOV(m_OldFOV);
    m_CurrentFOV = m_OldFOV / PHD_DEGREE;
}

static int32_t M_GetShiftSpeed(const int32_t val)
{
    return val * m_PhotoSpeed / (float)PHOTO_MAX_SPEED;
}

static int32_t M_GetRotSpeed(void)
{
    return MAX(PHD_DEGREE, M_GetShiftSpeed(PHOTO_ROT_SHIFT));
}

static void M_ShiftCamera(
    const int16_t angle, const int16_t elevation, const int32_t dy)
{
    const int32_t distance = M_GetShiftSpeed((WALL_L * 5.0) / LOGIC_FPS);
    const XYZ_32 shift = {
        .x = SHIFT_X(distance, elevation, angle),
        .z = SHIFT_Z(distance, elevation, angle),
        .y = SHIFT_Y(distance * dy, elevation),
    };
    SHIFT_POS(g_Camera.pos, shift);
    SHIFT_POS(g_Camera.target, shift);
}

static void M_RotateCamera(const int16_t angle, int16_t elevation)
{
    g_Camera.target_angle = angle;

    CLAMP(elevation, -PHOTO_MAX_PITCH_ROLL, PHOTO_MAX_PITCH_ROLL);
    g_Camera.target_elevation = elevation;

    const int32_t distance = g_Camera.target_distance;
    const XYZ_32 shift = {
        .x = SHIFT_X(distance, elevation, angle),
        .z = SHIFT_Z(distance, elevation, angle),
        .y = SHIFT_Y(-distance, elevation),
    };
    g_Camera.target.x = g_Camera.pos.x + shift.x;
    g_Camera.target.y = g_Camera.pos.y + shift.y;
    g_Camera.target.z = g_Camera.pos.z + shift.z;
}

static void M_RotateTarget(const int16_t angle)
{
    const PHD_ANGLE elevation = g_Camera.target_elevation;
    const int32_t distance = g_Camera.target_distance;
    const XYZ_32 shift = {
        .x = SHIFT_X(distance, elevation, angle),
        .z = SHIFT_Z(distance, elevation, angle),
        .y = SHIFT_Y(distance, elevation),
    };
    g_Camera.pos.x = g_Camera.target.x + shift.x;
    g_Camera.pos.y = g_Camera.target.y + shift.y;
    g_Camera.pos.z = g_Camera.target.z + shift.z;
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
        M_ShiftCamera(g_Camera.target_angle - PHD_90, 0, 0);
        result = true;
    } else if (g_Input.camera_right) {
        M_ShiftCamera(g_Camera.target_angle + PHD_90, 0, 0);
        result = true;
    }

    if (g_Input.camera_forward) {
        M_ShiftCamera(g_Camera.target_angle, g_Camera.target_elevation, -1);
        result = true;
    } else if (g_Input.camera_back) {
        M_ShiftCamera(
            g_Camera.target_angle + PHD_180, g_Camera.target_elevation, 1);
        result = true;
    }

    if (g_Input.camera_up) {
        M_ShiftCamera(
            g_Camera.target_angle, g_Camera.target_elevation + PHD_90, -1);
        result = true;
    } else if (g_Input.camera_down) {
        M_ShiftCamera(
            g_Camera.target_angle, g_Camera.target_elevation - PHD_90, -1);
        result = true;
    }

    return result;
}

static bool M_HandleRotationInputs(void)
{
    bool result = false;
    const int32_t rot_amount = M_GetRotSpeed();

    if (g_Input.forward) {
        M_RotateCamera(
            g_Camera.target_angle, g_Camera.target_elevation - rot_amount);
        result = true;
    } else if (g_Input.back) {
        M_RotateCamera(
            g_Camera.target_angle, g_Camera.target_elevation + rot_amount);
        result = true;
    }

    if (g_Input.left) {
        M_RotateCamera(
            g_Camera.target_angle - rot_amount, g_Camera.target_elevation);
        result = true;
    } else if (g_Input.right) {
        M_RotateCamera(
            g_Camera.target_angle + rot_amount, g_Camera.target_elevation);
        result = true;
    }

    if (g_Input.step_left) {
        g_Camera.roll -= rot_amount;
        result = true;
    } else if (g_Input.step_right) {
        g_Camera.roll += rot_amount;
        result = true;
    }
    CLAMP(g_Camera.roll, -PHOTO_MAX_PITCH_ROLL, PHOTO_MAX_PITCH_ROLL);

    return result;
}

static bool M_HandleTargetRotationInputs(void)
{
    bool result = false;
    if (g_InputDB.roll) {
        M_RotateTarget(g_Camera.target_angle - PHD_90);
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
    Viewport_SetFOV(m_CurrentFOV * PHD_DEGREE);
    Output_ApplyFOV();
    return true;
}

void Camera_EnterPhotoMode(void)
{
    m_OldFOV = Viewport_GetFOV();
    m_CurrentFOV = m_OldFOV / PHD_DEGREE;
    m_OldCamera = g_Camera;
    g_Camera.type = CAM_PHOTO_MODE;
    m_WorldBounds = Room_GetWorldBounds();
    M_UpdateCameraRooms();
}

void Camera_ExitPhotoMode(void)
{
    Viewport_SetFOV(m_OldFOV);
    M_ResetCamera();
}

void Camera_UpdatePhotoMode(void)
{
    const bool shift_input = g_Input.camera_forward || g_Input.camera_back
        || g_Input.camera_left || g_Input.camera_right || g_Input.camera_up
        || g_Input.camera_down;
    const bool rot_input =
        g_Input.left || g_Input.right || g_Input.forward || g_Input.back;
    const bool rot_target_input = g_InputDB.roll;
    const bool roll_input = g_Input.step_left || g_Input.step_right;

    PHD_ANGLE angles[2];
    Math_GetVectorAngles(
        g_Camera.target.x - g_Camera.pos.x, g_Camera.target.y - g_Camera.pos.y,
        g_Camera.target.z - g_Camera.pos.z, angles);
    g_Camera.target_angle = angles[0];
    g_Camera.target_elevation = angles[1];
    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.target_square = SQUARE(g_Camera.target_distance);

    if (g_InputDB.look) {
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

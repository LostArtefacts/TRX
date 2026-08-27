#include <trx/game/camera/photo_mode.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara/pose.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>

// clang-format off
#define M_MIN_FOV     10
#define M_MAX_FOV     150
#define M_ROT_SHIFT   (DEG_1 * 4) // = 728
#define M_MAX_SPEED   100
#define M_CLAMP_SHIFT STEP_L
// clang-format on

static int32_t m_PhotoSpeed = 0;
static int32_t m_OriginalFOV;
static FOV_MODE m_OriginalFOVMode;
static CAMERA_INFO m_OriginalCamera = {};
static int32_t m_CurrentFOV;
static FOV_MODE m_CurrentFOVMode;
static CAMERA_INFO m_StartingCamera = {};
static struct {
    bool is_chunky;
    int32_t fov;
    FOV_MODE fov_mode;
    CAMERA_INFO camera;
} m_PreviousState;
static BOUNDS_32 m_WorldBounds = {};

static void M_SyncCameraOrientationFromView(void)
{
    int16_t angles[2];
    Math_GetVectorAngles(
        g_Camera.target.x - g_Camera.pos.x, g_Camera.target.y - g_Camera.pos.y,
        g_Camera.target.z - g_Camera.pos.z, angles);
    g_Camera.target_angle = angles[0];
    g_Camera.target_elevation = angles[1];
}

static void M_RestoreSnapshotDriver(const CAMERA_INFO *const snapshot)
{
    const CAMERA_INFO live_camera = g_Camera;
    g_Camera = *snapshot;
    // Restore the camera owner/type and other flyby-managed state from the
    // entry snapshot, but preserve the live view that the flyby has already
    // advanced to while photo mode was open.
    g_Camera.pos = live_camera.pos;
    g_Camera.target = live_camera.target;
    g_Camera.shift = live_camera.shift;
    g_Camera.roll = live_camera.roll;
    g_Camera.underwater = live_camera.underwater;
    g_Camera.mic_pos = live_camera.mic_pos;
    g_Camera.interp = live_camera.interp;
}

static void M_ResetCamera(const bool exiting)
{
    CAMERA_INFO camera = g_Camera;
    g_Camera = exiting ? m_OriginalCamera : m_StartingCamera;
    // ensure Camera_EnsureEnvironment() picks up the flag change
    g_Camera.underwater = camera.underwater;
    g_Camera.interp = camera.interp;
    // Cinematic cameras control the FOV during their sequences, so avoid
    // resetting to user FOV on exit to prevent a 1-frame flicker.
    Viewport_AlterFOV(
        exiting && m_OriginalCamera.type != CAM_CINEMATIC ? -1 : m_OriginalFOV,
        m_OriginalFOVMode);
    m_CurrentFOV = m_OriginalFOV / DEG_1;
}

static int32_t M_GetShiftSpeed(const int32_t val)
{
    return val * m_PhotoSpeed / (float)M_MAX_SPEED;
}

static int32_t M_GetRotSpeed(void)
{
    return MAX(DEG_1, M_GetShiftSpeed(M_ROT_SHIFT));
}

// The step comes in along the camera's own axes, and has to land in the
// world's.
static XYZ_32 M_GetShift(const int32_t dx, const int32_t dy, const int32_t dz)
{
    return XYZ_32_Rotate(
        (XYZ_32) { .x = dx, .y = dy, .z = dz }, Camera_PhotoMode_GetRot());
}

static void M_ShiftCamera(const int32_t dx, const int32_t dy, const int32_t dz)
{
    const XYZ_32 shift = M_GetShift(dx, dy, dz);
    g_Camera.pos.pos = XYZ_32_Add(g_Camera.pos.pos, shift);
    g_Camera.target.pos = XYZ_32_Add(g_Camera.target.pos, shift);
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
        const XYZ_32 turned =
            XYZ_32_RotateYaw((XYZ_32) { .x = d_yaw, .z = d_pitch }, roll);
        yaw += turned.x;
        pitch += turned.z;
    } else {
        yaw += d_yaw;
        pitch += d_pitch;
    }
    roll += d_roll;

    if (pitch > DEG_90) {
        pitch = DEG_180 - pitch;
        yaw += DEG_180;
        roll += DEG_180;
    } else if (pitch < -DEG_90) {
        pitch = -DEG_180 - pitch;
        yaw += DEG_180;
        roll += DEG_180;
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
    g_Camera.target.pos = XYZ_32_Add(g_Camera.pos.pos, shift);
    if (g_Config.visuals.enable_photo_mode_collision) {
        Camera_LOSCheck(&g_Camera.pos, &g_Camera.target, M_CLAMP_SHIFT);
    }
}

static void M_RotateTarget(
    const int32_t d_yaw, const int32_t d_pitch, const int32_t d_roll)
{
    M_ApplyRotation(d_yaw, d_pitch, d_roll, false);
    const XYZ_32 shift = M_GetShift(0, 0, g_Camera.target_distance);
    g_Camera.pos.pos = XYZ_32_Subtract(g_Camera.target.pos, shift);
    if (g_Config.visuals.enable_photo_mode_collision) {
        Camera_LOSCheck(&g_Camera.target, &g_Camera.pos, M_CLAMP_SHIFT);
    }
}

static void M_Clamp(GAME_VECTOR *const pos)
{
    Camera_Collide(pos, M_CLAMP_SHIFT, false);
    int16_t room_num = pos->room_num;
    const SECTOR *const sector =
        Room_GetSector((XYZ_32) { pos->x, MAX_HEIGHT, pos->z }, &room_num);
    const ROOM *const room = Room_Get(room_num);
    if (room->flags.swamp) {
        CLAMPG(pos->y, room->max_ceiling - M_CLAMP_SHIFT);
        Room_GetSector(pos->pos, &pos->room_num);
    }
}

static void M_ClampCameraPos(void)
{
    if (g_Config.visuals.enable_photo_mode_collision) {
        M_Clamp(&g_Camera.pos);
        M_Clamp(&g_Camera.target);
        return;
    }

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

static bool M_CameraInsideRoom(const XYZ_32 pos, const int16_t room_num)
{
    return Room_PointInside(Room_Get(room_num), pos);
}

static void M_UpdateCameraRooms(void)
{
    Room_GetSector(g_Camera.pos.pos, &g_Camera.pos.room_num);
    Room_GetSector(g_Camera.target.pos, &g_Camera.target.room_num);
    if (!M_CameraInsideRoom(g_Camera.pos.pos, g_Camera.pos.room_num)) {
        const int16_t pos_room_num = Room_GetIndexFromPos(g_Camera.pos.pos);
        const int16_t tar_room_num = Room_GetIndexFromPos(g_Camera.target.pos);

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

    Camera_EnsureEnvironment();
}

static bool M_HandleShiftInputs(void)
{
    bool result = false;

    const int32_t distance = M_GetShiftSpeed((WALL_L * 5.0) / LOGIC_FPS);
    if (g_Input.camera_left) {
        M_ShiftCamera(-distance, 0, 0);
        result = true;
    } else if (g_Input.camera_right) {
        M_ShiftCamera(distance, 0, 0);
        result = true;
    }

    if (g_Input.camera_forward) {
        M_ShiftCamera(0, 0, distance);
        result = true;
    } else if (g_Input.camera_back) {
        M_ShiftCamera(0, 0, -distance);
        result = true;
    }

    if (!g_Input.slow && g_Input.camera_up) {
        M_ShiftCamera(0, -distance, 0);
        result = true;
    } else if (!g_Input.slow && g_Input.camera_down) {
        M_ShiftCamera(0, distance, 0);
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

    if (g_Input.slow && g_Input.camera_up) {
        M_RotateCamera(0, 0, -M_GetRotSpeed());
        result = true;
    } else if (g_Input.slow && g_Input.camera_down) {
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
    CLAMP(m_CurrentFOV, M_MIN_FOV, M_MAX_FOV);
    Viewport_AlterFOV(m_CurrentFOV * DEG_1, m_CurrentFOVMode);
    return true;
}

XYZ_16 Camera_PhotoMode_GetRot(void)
{
    return (XYZ_16) {
        .x = g_Camera.target_elevation,
        .y = g_Camera.target_angle,
        .z = g_Camera.roll,
    };
}

void Camera_PhotoMode_Enter(void)
{
    m_OriginalCamera = g_Camera;

    M_SyncCameraOrientationFromView();
    g_Camera.target_distance = CAMERA_DEFAULT_DISTANCE;
    g_Camera.target_square = SQUARE(g_Camera.target_distance);

    g_Camera.target.room_num = g_Camera.pos.room_num;
    Room_GetSector(g_Camera.target.pos, &g_Camera.target.room_num);

    m_StartingCamera = g_Camera;

    m_OriginalFOV = Viewport_GetEffectiveFOV();
    m_OriginalFOVMode = Viewport_GetFOVMode();
    m_CurrentFOV = m_OriginalFOV / DEG_1;
    m_CurrentFOVMode = m_OriginalFOVMode;
    g_Camera.type = CAM_PHOTO_MODE;
    const int32_t border = WALL_L * 5;
    m_WorldBounds = Room_GetWorldBounds();
    m_WorldBounds.min.x -= border;
    m_WorldBounds.min.y -= border;
    m_WorldBounds.min.z -= border;
    m_WorldBounds.max.x += border;
    m_WorldBounds.max.y += border;
    m_WorldBounds.max.z += border;
    M_UpdateCameraRooms();
}

void Camera_PhotoMode_Exit(void)
{
    Lara_Pose_Clear();
    if (Camera_FlybyMode_IsActive()) {
        // The snapshot from when photo mode opened still has the correct
        // flyby-owned driver state, but its view data is stale. Rewind only
        // the driver half so the flyby can finish normally without snapping
        // the visible camera back to the old entry position.
        M_RestoreSnapshotDriver(&m_OriginalCamera);
        return;
    }
    Viewport_AlterFOV(m_OriginalFOV, m_OriginalFOVMode);
    M_ResetCamera(true);
}

void Camera_PhotoMode_Update(void)
{
    M_HandleFOVInputs();

    bool changed = false;

    if (g_InputDB.camera_reset) {
        M_ResetCamera(false);
        g_Camera.type = CAM_PHOTO_MODE;
        changed = true;
    }

    changed |= M_HandleShiftInputs();
    changed |= M_HandleRotationInputs();

    if (changed) {
        m_PhotoSpeed++;
        CLAMPG(m_PhotoSpeed, M_MAX_SPEED);
    } else {
        m_PhotoSpeed = 0;
    }

    changed |= M_HandleTargetRotationInputs();

    if (changed) {
        g_Camera.mic_pos = g_Camera.pos;
        M_ClampCameraPos();
        M_UpdateCameraRooms();
    }
}

void Camera_PhotoMode_UpdateFOV(void)
{
    M_HandleFOVInputs();
}

void Camera_PhotoMode_Pause(void)
{
    m_PreviousState.camera = g_Camera;
    m_PreviousState.is_chunky = Camera_IsChunky();
    m_PreviousState.fov = Viewport_GetSystemFOV();
    m_PreviousState.fov_mode = Viewport_GetFOVMode();
    // A running flyby already carries its own live camera state (including
    // interpolation history); rebasing it onto the stale entry-time snapshot
    // causes a one-frame interpolation snap when the result is kept in
    // Camera_PhotoMode_Resume().
    if (!Camera_FlybyMode_IsActive()) {
        g_Camera = m_OriginalCamera;
    }
}

void Camera_PhotoMode_Resume(void)
{
    // A flyby drives the camera itself, so the frame we just simulated is
    // the only thing moving it forward. Restoring the pre-step snapshot
    // here would discard that progress and make advancing a frame look
    // like a no-op while a flyby plays.
    if (Camera_FlybyMode_IsActive()) {
        // Keep photo mode's local movement axes aligned with the live flyby
        // view that just advanced; otherwise left/right movement can use the
        // stale pre-step orientation basis.
        M_SyncCameraOrientationFromView();
    } else {
        if (m_OriginalCamera.type == CAM_FLYBY_MODE) {
            // The flyby finished between pausing and resuming photo mode.
            // Resync the original camera to the one generated post-flyby.
            m_OriginalCamera = g_Camera;
        }
        g_Camera = m_PreviousState.camera;
    }
    Camera_SetChunky(m_PreviousState.is_chunky);
    Viewport_AlterFOV(m_PreviousState.fov, m_PreviousState.fov_mode);
}

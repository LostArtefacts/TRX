#include <trx/game/camera/flyby_mode.h>

#include <trx/game/camera.h>
#include <trx/game/game_flow.h>
#include <trx/game/lara.h>
#include <trx/game/lua/events.h>
#include <trx/game/output/overlay.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>

#define M_NO_SEQUENCE (-1)
#define M_LETTERBOX (16.0f / 480.0f)

static int32_t m_CurrentSequence = M_NO_SEQUENCE;
static ITEM m_TriggerItem = {
    .object_id = O_CAMERA_TARGET,
};

static struct {
    struct {
        GAME_VECTOR camera_pos;
        GAME_VECTOR camera_target;
        int16_t fov;
        FOV_MODE fov_mode;
    } initial;
    struct {
        int32_t camera_idx;
        int32_t spline_pos;
    } current;
    struct {
        bool spline_from_game;
        bool spline_to_game;
        bool test_triggers;
        bool pending_trigger_check;
    } flags;
    SPLINE_DATA data;
    int16_t timer;
} m_State = {};

static int32_t M_GetLastCamera(const FLYBY_SEQUENCE *const sequence)
{
    return sequence->camera_idx + sequence->num_cameras - 1;
}

static void M_SlideLetterboxIn(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->type != GFL_TITLE) {
        Output_Overlay_SlideLetterbox(M_LETTERBOX);
    }
}

static void M_PrepareSequence(const int32_t sequence_idx)
{
    m_CurrentSequence = sequence_idx;
    m_State.initial.camera_pos = g_Camera.pos;
    m_State.initial.camera_target = g_Camera.target;
    m_State.initial.fov = Viewport_GetEffectiveFOV();
    m_State.initial.fov_mode = Viewport_GetFOVMode();
    m_State.flags.spline_from_game = false;
    m_State.flags.spline_to_game = false;
    m_State.flags.test_triggers = false;
    m_State.flags.pending_trigger_check = false;
    m_State.timer = 0;

    const FLYBY_SEQUENCE *const sequence =
        Camera_GetSequence(m_CurrentSequence);
    const FLYBY_CAMERA *const camera =
        Camera_GetFlybyCamera(sequence->camera_idx);
    const int32_t last_camera_idx = M_GetLastCamera(sequence);

    g_Camera.type = CAM_FLYBY_MODE;
    g_Camera.pos.room_num = camera->room_num;

    m_State.current.camera_idx = sequence->camera_idx;
    m_State.current.spline_pos = 0;
    if (camera->flags.lara_control_off) {
        Lara_SetControllable(false);
        M_SlideLetterboxIn();
    }

    if (camera->flags.track_path) {
        int32_t slot = 0;
        int32_t camera_idx = sequence->camera_idx;
        Spline_SetupData(&m_State.data, slot, camera_idx);
        slot++;

        for (int32_t i = 0; i < sequence->num_cameras; i++) {
            Spline_SetupData(&m_State.data, slot, camera_idx);
            slot++;
            camera_idx++;
        }

        Spline_SetupData(&m_State.data, slot, last_camera_idx);
        const ITEM *const lara_item = Lara_GetItem();
        m_State.current.spline_pos = Spline_GetNearestPosition(
            lara_item->pos, &m_State.data, sequence->num_cameras + 2);
    } else if (camera->flags.snap_from_game) {
        int32_t slot = 0;
        int32_t camera_idx = sequence->camera_idx;
        Spline_SetupData(&m_State.data, slot, camera_idx);
        slot++;

        while (slot < 4) {
            if (camera_idx > last_camera_idx) {
                camera_idx = sequence->camera_idx;
            }

            Spline_SetupData(&m_State.data, slot, camera_idx);
            slot++;
            camera_idx++;
        }

        m_State.current.camera_idx++;
        if (m_State.current.camera_idx > last_camera_idx) {
            m_State.current.camera_idx = sequence->camera_idx;
        }

        if (camera->flags.test_triggers) {
            m_State.flags.test_triggers = true;
        }
    } else {
        m_State.flags.spline_from_game = true;

        m_State.data.pos.x[0] = m_State.initial.camera_pos.x;
        m_State.data.pos.y[0] = m_State.initial.camera_pos.y;
        m_State.data.pos.z[0] = m_State.initial.camera_pos.z;
        m_State.data.target.x[0] = m_State.initial.camera_target.x;
        m_State.data.target.y[0] = m_State.initial.camera_target.y;
        m_State.data.target.z[0] = m_State.initial.camera_target.z;
        m_State.data.roll[0] = 0;
        m_State.data.fov[0] = m_State.initial.fov;
        m_State.data.speed[0] = camera->speed; // missing in OG
        m_State.data.pos.x[1] = m_State.data.pos.x[0];
        m_State.data.pos.y[1] = m_State.data.pos.y[0];
        m_State.data.pos.z[1] = m_State.data.pos.z[0];
        m_State.data.target.x[1] = m_State.data.target.x[0];
        m_State.data.target.y[1] = m_State.data.target.y[0];
        m_State.data.target.z[1] = m_State.data.target.z[0];
        m_State.data.roll[1] = m_State.data.roll[0];
        m_State.data.fov[1] = m_State.data.fov[0];
        m_State.data.speed[1] = m_State.data.speed[0];

        int32_t camera_idx = sequence->camera_idx;
        Spline_SetupData(&m_State.data, 2, camera_idx);
        camera_idx++;
        if (camera_idx > last_camera_idx) {
            camera_idx = sequence->camera_idx;
        }
        Spline_SetupData(&m_State.data, 3, camera_idx);
    }
}

static void M_PrepareSplineToGame(void)
{
    m_State.flags.spline_to_game = true;
    m_State.current.camera_idx--;
    Spline_SetupData(&m_State.data, 0, m_State.current.camera_idx - 1);
    Spline_SetupData(&m_State.data, 1, m_State.current.camera_idx);

    CAMERA_INFO cam_info = g_Camera;
    g_Camera.type = CAM_CHASE;
    g_Camera.speed = 1;
    Camera_Update();

    m_State.initial.camera_pos = g_Camera.pos;
    m_State.initial.camera_target = g_Camera.target;
    m_State.data.pos.x[2] = g_Camera.pos.x;
    m_State.data.pos.y[2] = g_Camera.pos.y;
    m_State.data.pos.z[2] = g_Camera.pos.z;
    m_State.data.target.x[2] = g_Camera.target.x;
    m_State.data.target.y[2] = g_Camera.target.y;
    m_State.data.target.z[2] = g_Camera.target.z;
    m_State.data.fov[2] = Viewport_GetEffectiveFOV();
    m_State.data.roll[2] = 0;
    m_State.data.speed[2] = m_State.data.speed[1];
    m_State.data.pos.x[3] = g_Camera.pos.x;
    m_State.data.pos.y[3] = g_Camera.pos.y;
    m_State.data.pos.z[3] = g_Camera.pos.z;
    m_State.data.target.x[3] = g_Camera.target.x;
    m_State.data.target.y[3] = g_Camera.target.y;
    m_State.data.target.z[3] = g_Camera.target.z;
    m_State.data.speed[3] = m_State.data.speed[1] >> 1;
    m_State.data.fov[3] = m_State.data.fov[2];

    g_Camera = cam_info;
}

static void M_TestTriggers(void)
{
    if (!m_State.flags.test_triggers) {
        return;
    }

    m_TriggerItem.pos = g_Camera.pos.pos;
    m_TriggerItem.room_num = g_Camera.pos.room_num;

    // The camera stands in for a heavy object while it tests, so the triggers
    // it runs do not also reach for a fixed camera.
    const CAMERA_TYPE camera_type = g_Camera.type;
    g_Camera.type = CAM_HEAVY;

    // There is no player on the title level to walk onto a pad or a plain
    // trigger, so the flyby answers for both kinds there.
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level != nullptr && level->type == GFL_TITLE) {
        Room_TestTriggersEx(&m_TriggerItem, false);
    }
    Room_TestTriggersEx(&m_TriggerItem, true);

    g_Camera.type = camera_type;
    m_State.flags.test_triggers = false;
}

bool Camera_FlybyMode_Activate(const int32_t sequence_idx, const bool one_shot)
{
    FLYBY_SEQUENCE *const sequence = Camera_GetSequence(sequence_idx);
    if (sequence == nullptr) {
        return false;
    }

    if (sequence_idx == m_CurrentSequence) {
        m_State.flags.pending_trigger_check = false;
        return false;
    } else if (m_CurrentSequence != M_NO_SEQUENCE) {
        return false;
    }

    if (sequence->camera_idx == NO_CAMERA) {
        return false;
    }

    if (sequence->one_shot) {
        return false;
    }

    if (one_shot) {
        sequence->one_shot = true;
    }

    M_PrepareSequence(sequence_idx);
    return true;
}

bool Camera_FlybyMode_IsActive(void)
{
    return m_CurrentSequence != M_NO_SEQUENCE;
}

bool Camera_Flybymode_Cancel(const bool force)
{
    if (m_CurrentSequence == M_NO_SEQUENCE) {
        return false;
    }

    const FLYBY_SEQUENCE *const sequence =
        Camera_GetSequence(m_CurrentSequence);
    const FLYBY_CAMERA *const first_camera =
        Camera_GetFlybyCamera(sequence->camera_idx);
    if (first_camera->flags.track_path
        || (first_camera->flags.no_break && !force)) {
        return false;
    }

    const int32_t last_camera_idx = M_GetLastCamera(sequence);
    for (int32_t i = m_State.current.camera_idx; i <= last_camera_idx; i++) {
        const FLYBY_CAMERA *const camera = Camera_GetFlybyCamera(i);
        if (!camera->flags.test_triggers) {
            continue;
        }

        g_Camera.pos.pos = camera->pos;
        g_Camera.pos.room_num = camera->room_num;
        m_State.flags.test_triggers = true;
        M_TestTriggers();
    }

    Camera_FlybyMode_Deactivate();
    Camera_ResetPosition();
    return true;
}

void Camera_FlybyMode_Reset(void)
{
    m_CurrentSequence = M_NO_SEQUENCE;
    // Left standing, this would send the next update down the hand-back path
    // and put the camera on Lara, even though no sequence is running.
    m_State.flags.pending_trigger_check = false;
}

void Camera_FlybyMode_Deactivate(void)
{
    Camera_FlybyMode_Reset();
    Lara_SetControllable(true);

    g_Camera.type = CAM_CHASE;
    g_Camera.roll = 0;
    Viewport_AlterFOV(m_State.initial.fov, m_State.initial.fov_mode);
    if (!m_State.flags.spline_to_game) {
        g_Camera.speed = 1;
        g_Camera.pos = m_State.initial.camera_pos;
        g_Camera.target = m_State.initial.camera_target;
        g_Camera.interp.prev.pos = g_Camera.pos.pos;
        g_Camera.interp.prev.target = g_Camera.target.pos;
        Camera_Update();
    }
    Output_Overlay_SlideLetterbox(0.0f);
}

void Camera_FlybyMode_Update(void)
{
    if (m_State.flags.pending_trigger_check) {
        // Lara's no longer on a trigger for a track_path
        g_Camera.speed = Camera_GetChaseSpeed();
        m_State.flags.spline_to_game = true;
        Camera_FlybyMode_Deactivate();
        return;
    }

    const FLYBY_SEQUENCE *const sequence =
        Camera_GetSequence(m_CurrentSequence);
    if (sequence == nullptr) {
        return;
    }

    const FLYBY_CAMERA *const first_camera =
        Camera_GetFlybyCamera(sequence->camera_idx);
    const FLYBY_CAMERA *const current_camera =
        Camera_GetFlybyCamera(m_State.current.camera_idx);
    const int32_t spline_count =
        first_camera->flags.track_path ? sequence->num_cameras + 2 : 4;
    const ITEM *const lara_item = Lara_GetItem();

    const XYZ_32 pos = {
        .x = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.pos.x, spline_count),
        .y = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.pos.y, spline_count),
        .z = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.pos.z, spline_count),
    };
    const XYZ_32 target = {
        .x = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.target.x, spline_count),
        .y = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.target.y, spline_count),
        .z = Spline_Calculate(
            m_State.current.spline_pos, m_State.data.target.z, spline_count),
    };

    const int32_t speed = Spline_Calculate(
        m_State.current.spline_pos, m_State.data.speed, spline_count);
    const int32_t roll = Spline_Calculate(
        m_State.current.spline_pos, m_State.data.roll, spline_count);
    const int32_t fov = Spline_Calculate(
        m_State.current.spline_pos, m_State.data.fov, spline_count);

    // TODO: handle fade_in_screen and fade_out_screen

    if (first_camera->flags.track_path) {
        const int32_t cp = Spline_GetNearestPosition(
            lara_item->pos, &m_State.data, spline_count);
        m_State.current.spline_pos += (cp - m_State.current.spline_pos) >> 5;
        if (first_camera->flags.snap_to_game
            && ABS(cp - m_State.current.spline_pos) > 0x8000) {
            m_State.current.spline_pos = cp;
        }
    } else if (m_State.timer == 0) {
        m_State.current.spline_pos += speed;
    }

    g_Camera.pos.pos = pos;
    if (first_camera->flags.target_lara || first_camera->flags.track_path) {
        g_Camera.target.pos = lara_item->pos;
    } else {
        g_Camera.target.pos = target;
    }

    if (current_camera->flags.target_item) {
        const ITEM *const item = Item_Get(current_camera->timer);
        if (item != nullptr) {
            g_Camera.target.pos = item->pos;
        }
    }

    const int16_t room_num = Room_FindByTraversal(
        current_camera->pos, pos, current_camera->room_num);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }
    g_Camera.target.room_num = g_Camera.pos.room_num;
    Room_GetSector(g_Camera.target.pos, &g_Camera.target.room_num);
    g_Camera.shift = 0;
    g_Camera.roll = roll;
    Viewport_AlterFOV(fov, FOV_MODE_GAME);
    Camera_UpdateMicPosition();

    M_TestTriggers();

    if (first_camera->flags.track_path) {
        // Switch off control until the next control run tests if Lara is still
        // on a valid trigger.
        m_State.flags.pending_trigger_check = true;
        return;
    }

    if (m_State.current.spline_pos <= SPLINE_ONE - speed) {
        return;
    }

    if (current_camera->flags.test_triggers) {
        m_State.flags.test_triggers = true;
    }

    if (current_camera->flags.hold) {
        if (m_State.timer != 0) {
            m_State.timer--;
        } else {
            m_State.timer = current_camera->timer >> 4;
        }
    }

    if (m_State.timer == 0) {
        m_State.current.spline_pos = 0;

        int32_t next_camera = 0;
        if (m_State.current.camera_idx == sequence->camera_idx) {
            next_camera = M_GetLastCamera(sequence);
        } else {
            next_camera = m_State.current.camera_idx - 1;
        }

        int32_t slot = 1;
        if (m_State.flags.spline_from_game) {
            m_State.flags.spline_from_game = false;
            next_camera = sequence->camera_idx - 1;
        } else {
            if (current_camera->flags.lara_control_on) {
                Lara_SetControllable(true);
            }
            if (current_camera->flags.lara_control_off) {
                Lara_SetControllable(false);
                M_SlideLetterboxIn();
            }

            slot = 0;
            if (current_camera->flags.jump_to_camera) {
                next_camera =
                    sequence->camera_idx + (current_camera->timer & 0xF);
                m_State.current.camera_idx = next_camera;
                Spline_SetupData(&m_State.data, slot, next_camera);
                slot = 1;
            }

            Spline_SetupData(&m_State.data, slot, next_camera);
            slot++;
        }

        next_camera++;

        while (slot < 4) {
            if (first_camera->flags.loop) {
                if (next_camera > M_GetLastCamera(sequence)) {
                    next_camera = sequence->camera_idx;
                }
            } else if (next_camera > M_GetLastCamera(sequence)) {
                next_camera = M_GetLastCamera(sequence);
            }

            Spline_SetupData(&m_State.data, slot, next_camera);
            next_camera++;
            slot++;
        }

        m_State.current.camera_idx++;

        if (m_State.current.camera_idx > M_GetLastCamera(sequence)) {
            if (first_camera->flags.loop) {
                m_State.current.camera_idx = sequence->camera_idx;
            } else if (
                first_camera->flags.snap_to_game
                || m_State.flags.spline_to_game) {
                M_TestTriggers();
                const int32_t finished = m_CurrentSequence;
                Camera_FlybyMode_Deactivate();
                LUA_FireEventInt32(LUA_EVENT_FLYBY_END, finished);
            } else {
                M_PrepareSplineToGame();
            }
        }
    }
}

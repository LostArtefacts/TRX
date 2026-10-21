#include <trx/game/camera/binoculars.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/fx/laser_sight.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/laser_sight.h>
#include <trx/game/gun/registry.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/game/viewport.h>

#define M_MIN_RANGE 128
#define M_MAX_RANGE 1536
#define M_ZOOM_STEP 64
#define M_ZOOM_STEP_SLOW 32
#define M_MAX_PITCH 13650
#define M_MAX_YAW 14560
#define M_MAX_HEAD_ROT_Y 8008
#define M_MIN_HEAD_ROT_X (-6370)
#define M_MAX_HEAD_ROT_X 5460
#define M_HEAD_TURN 364
#define M_TARGET_DIST 20736
// How many ticks the look key stays held with a weapon ready before the
// laser sight view opens. A shorter press is the original's target switch.
#define M_LASER_SIGHT_HOLD_TICKS 6
#define M_TORCH_BRIGHTNESS 192
#define M_TORCH_STEPS 32
#define M_FOR_EACH_EXIT_INPUT(X)                                               \
    X(Draw, draw)                                                              \
    X(Look, look)                                                              \
    X(Option, option)

static bool m_Active = false;
// Whether the view is the laser sight's rather than the binoculars'.
static bool m_LaserSight = false;
// Whether the laser sight fires on the next update, and how many ticks it
// waits before it can fire again.
static bool m_FirePending = false;
static int32_t m_FireDelay = 0;
static int32_t m_LookHeldTicks = 0;
static bool m_Pending = false;
static bool m_JustEntered = false;
static bool m_TorchActive = false;
// Holds the torch off while the action key that was held as the binoculars
// came up stays held, such as the key that chose them in the inventory.
static bool m_TorchHeldOff = false;
// Holds the zoom off while a zoom key that was held as the laser sight view
// opened stays held, such as the crouch key that keeps Lara crouched.
static bool m_ZoomHeldOff = false;
#define M_DECLARE_SUPPRESS_FLAG(name, field)                                   \
    static bool m_Suppress##name = false;
M_FOR_EACH_EXIT_INPUT(M_DECLARE_SUPPRESS_FLAG)
#undef M_DECLARE_SUPPRESS_FLAG
static int32_t m_Range = M_MIN_RANGE;
static int32_t m_OriginalFOV;
static FOV_MODE m_OriginalFOVMode;
static CAMERA_INFO m_OriginalCamera = {};

static bool M_CanEnter(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    return lara_item->hit_points > 0 && lara_info->gun_status == LGS_ARMLESS
        && lara_info->water_status == LWS_ABOVE_WATER
        && Lara_Vehicle_GetItem() == nullptr
        && lara_item->current_anim_state == LS(LS_STOP)
        && g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_PHOTO_MODE;
}

// Whether Lara can look through the laser sight of the weapon she holds:
// she holds it ready, the weapon carries its attachment, and she stands or
// crouches still. The original refuses while the crouch key is held, and the
// view instead holds off the zoom until that key is let go.
static bool M_CanEnterLaserSight(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    return lara_item->hit_points > 0 && lara_info->gun_status == LGS_READY
        && Inv_HasAttachment(lara_info->gun_type)
        && lara_info->water_status == LWS_ABOVE_WATER
        && Lara_Vehicle_GetItem() == nullptr
        && (lara_item->current_anim_state == LS(LS_STOP)
            || lara_item->current_anim_state == LS(LS_CROUCH_IDLE))
        && g_Camera.type != CAM_CINEMATIC && g_Camera.type != CAM_PHOTO_MODE;
}

static void M_Enter(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    m_OriginalCamera = g_Camera;
    m_OriginalFOV = Viewport_GetEffectiveFOV();
    m_OriginalFOVMode = Viewport_GetFOVMode();
    m_Range = M_MIN_RANGE;
    m_TorchActive = false;
    m_TorchHeldOff = g_Input.action;
    m_JustEntered = true;
    m_Active = true;
    lara_info->gun_status = LGS_HANDS_BUSY;
    g_Camera.type = CAM_BINOCULARS;
}

static void M_RefuseRequest(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->hit_points > 0 && Lara_IsControllable()
        && !lara->extra_anim) {
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_ALWAYS);
    }
}

// The look key closes the laser sight view as it is pressed again where the
// view toggles, and as it is let go otherwise, as in the original.
static bool M_ShouldExitLaserSight(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const bool look_closes = g_Config.gameplay.enable_laser_sight_toggle
        ? g_InputDB.look
        : !g_Input.look;
    return look_closes || g_Input.option || g_Input.save || g_Input.load
        || g_InputDB.quick_save || g_InputDB.quick_load
        || lara_item->hit_points <= 0 || Lara_Vehicle_GetItem() != nullptr
        || lara_info->gun_status != LGS_READY
        || !Inv_HasAttachment(lara_info->gun_type);
}

static bool M_ShouldExit(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    return g_InputDB.draw || g_InputDB.look || g_Input.option || g_Input.save
        || g_Input.load || g_InputDB.quick_save || g_InputDB.quick_load
        || g_InputDB.use_binoculars || lara_item->hit_points <= 0
        || Lara_Vehicle_GetItem() != nullptr;
}

static void M_HandleLookInput(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    if (g_Input.left) {
        const int16_t turn = M_HEAD_TURN * (1792 - m_Range) / 1536;
        if (lara_info->head_rot.y > -M_MAX_HEAD_ROT_Y) {
            lara_info->head_rot.y -= turn;
        }
    } else if (g_Input.right) {
        const int16_t turn = M_HEAD_TURN * (1792 - m_Range) / 1536;
        if (lara_info->head_rot.y < M_MAX_HEAD_ROT_Y) {
            lara_info->head_rot.y += turn;
        }
    }

    const bool inverted = g_Config.gameplay.enable_inverted_look;
    const bool tilt_up = inverted ? g_Input.back : g_Input.forward;
    const bool tilt_down = inverted ? g_Input.forward : g_Input.back;
    if (tilt_up) {
        const int16_t turn = M_HEAD_TURN * (1792 - m_Range) / 3072;
        if (lara_info->head_rot.x > M_MIN_HEAD_ROT_X) {
            lara_info->head_rot.x -= turn;
        }
    } else if (tilt_down) {
        const int16_t turn = M_HEAD_TURN * (1792 - m_Range) / 3072;
        if (lara_info->head_rot.x < M_MAX_HEAD_ROT_X) {
            lara_info->head_rot.x += turn;
        }
    }
}

static void M_HandleZoomInput(void)
{
    if (m_ZoomHeldOff) {
        m_ZoomHeldOff = g_Input.sprint || g_Input.crouch;
        return;
    }
    const int32_t step = g_Input.slow ? M_ZOOM_STEP_SLOW : M_ZOOM_STEP;
    if (g_Input.sprint) {
        m_Range -= step;
    } else if (g_Input.crouch) {
        m_Range += step;
    }
    CLAMP(m_Range, M_MIN_RANGE, M_MAX_RANGE);
}

static void M_EmitTorch(const XYZ_32 start, const XYZ_32 end)
{
    const ITEM *const lara_item = Lara_GetItem();
    const XYZ_32 step = {
        .x = (end.x - start.x) >> 5,
        .y = (end.y - start.y) >> 5,
        .z = (end.z - start.z) >> 5,
    };
    static const int32_t offs[5] = { 0, -0x4000, -0x4001, 0x4000, 0x4001 };
    const int16_t y_rot = Lara_GetLaraInfo()->head_rot.y;

    XYZ_32 pos = start;
    int32_t brightness = M_TORCH_BRIGHTNESS;
    int32_t falloff = 15;
    int32_t skip = 0;

    for (int32_t i = 0; i < M_TORCH_STEPS; i++) {
        if (skip == 0) {
            // Probe the beam cross-section; obstruction ends the beam.
            int32_t j;
            for (j = 0; j < 5; j++) {
                XYZ_32 test;
                if (offs[j] != 0) {
                    test = XYZ_32_OffsetYaw(pos, offs[j] + y_rot, falloff << 7);
                    test.y = (offs[j] & 1) != 0 ? pos.y - (falloff << 7)
                                                : pos.y + (falloff << 7);
                } else {
                    test = pos;
                }

                int16_t room_num = lara_item->room_num;
                const SECTOR *const sector = Room_GetSector(test, &room_num);
                const int32_t h = Room_GetHeight(sector, test);
                const int32_t c = Room_GetCeiling(sector, test);
                if (h == NO_HEIGHT || c == NO_HEIGHT || c >= h || test.y < c
                    || h < test.y) {
                    break;
                }
            }

            if (j < 5) {
                Output_AddDynamicLightRGB(
                    pos, falloff,
                    (RGB_888) { brightness, brightness, brightness >> 1 });
                skip = 5;
            }
        }

        if (skip > 0) {
            skip--;
        }

        brightness -= 7;
        if (brightness < 8) {
            break;
        }
        if (falloff < 31) {
            falloff += 2;
        }
        pos.x += step.x;
        pos.y += step.y;
        pos.z += step.z;
    }
}

// Swallow the key that closed the binoculars until it is released, so that
// it does not trigger an immediate action (e.g. drawing a weapon, entering
// look mode, or opening the inventory ring).
static void M_SuppressExitInputs(void)
{
#define M_SUPPRESS_EXIT_INPUT(name, field)                                     \
    if (m_Suppress##name) {                                                    \
        if (g_Input.field) {                                                   \
            g_Input.field = 0;                                                 \
            g_InputDB.field = 0;                                               \
        } else {                                                               \
            m_Suppress##name = false;                                          \
        }                                                                      \
    }
    M_FOR_EACH_EXIT_INPUT(M_SUPPRESS_EXIT_INPUT)
#undef M_SUPPRESS_EXIT_INPUT
}

// Fires the weapon through the laser sight while ACTION is held, as often as
// the attachment allows and while the weapon has rounds left.
static void M_HandleLaserSightFire(void)
{
    if (m_FireDelay > 0) {
        m_FireDelay--;
    }
    const LARA_GUN_TYPE gun_type = Lara_GetLaraInfo()->gun_type;
    if (!g_Input.action || m_FireDelay > 0 || !Gun_HasRoundsLeft(gun_type)) {
        return;
    }
    const WEAPON_INFO *const weapon = Gun_Registry_Get(gun_type);
    m_FireDelay = weapon->attachment.fire_delay;
    m_FirePending = true;
    for (int32_t i = 0; i < Gun_GetRoundsPerShot(gun_type); i++) {
        Gun_SpendRound(gun_type);
    }
    Stats_AddAmmoUsed();
    Sound_Effect(weapon->sample_num, nullptr, SPM_NORMAL);
}

static void M_EnterLaserSight(void)
{
    m_LookHeldTicks = 0;
    m_OriginalCamera = g_Camera;
    m_OriginalFOV = Viewport_GetEffectiveFOV();
    m_OriginalFOVMode = Viewport_GetFOVMode();
    m_Range = M_MIN_RANGE;
    m_TorchActive = false;
    m_FirePending = false;
    m_FireDelay = 0;
    m_ZoomHeldOff = g_Input.sprint || g_Input.crouch;
    m_JustEntered = true;
    m_Active = true;
    m_LaserSight = true;
    g_Camera.type = CAM_BINOCULARS;
}

void Camera_Binoculars_Reset(void)
{
    m_Active = false;
    m_LaserSight = false;
    m_FirePending = false;
    m_FireDelay = 0;
    m_Pending = false;
    m_TorchActive = false;
    m_TorchHeldOff = false;
    m_ZoomHeldOff = false;
#define M_RESET_SUPPRESS_FLAG(name, field) m_Suppress##name = false;
    M_FOR_EACH_EXIT_INPUT(M_RESET_SUPPRESS_FLAG)
#undef M_RESET_SUPPRESS_FLAG
    m_Range = M_MIN_RANGE;
}

void Camera_Binoculars_Request(void)
{
    m_Pending = true;
}

void Camera_Binoculars_Control(void)
{
    if (!m_Active) {
        M_SuppressExitInputs();
        if (m_Pending) {
            m_Pending = false;
            if (M_CanEnter()) {
                M_Enter();
            } else {
                M_RefuseRequest();
            }
        } else if (g_Input.look && M_CanEnterLaserSight()) {
            m_LookHeldTicks++;
            if (m_LookHeldTicks >= M_LASER_SIGHT_HOLD_TICKS) {
                M_EnterLaserSight();
                g_Input = (INPUT_STATE) {};
                g_InputDB = (INPUT_STATE) {};
            }
        } else {
            m_LookHeldTicks = 0;
        }
        return;
    }

    if (m_LaserSight) {
        if (M_ShouldExitLaserSight()) {
            Camera_Binoculars_Exit();
#define M_CAPTURE_EXIT_INPUT(name, field) m_Suppress##name = g_Input.field;
            M_FOR_EACH_EXIT_INPUT(M_CAPTURE_EXIT_INPUT)
#undef M_CAPTURE_EXIT_INPUT
            M_SuppressExitInputs();
            return;
        }
        g_Camera.type = CAM_BINOCULARS;
        M_HandleLookInput();
        M_HandleZoomInput();
        M_HandleLaserSightFire();
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
        return;
    }

    if (M_ShouldExit()) {
        Camera_Binoculars_Exit();
#define M_CAPTURE_EXIT_INPUT(name, field) m_Suppress##name = g_Input.field;
        M_FOR_EACH_EXIT_INPUT(M_CAPTURE_EXIT_INPUT)
#undef M_CAPTURE_EXIT_INPUT
        M_SuppressExitInputs();
        return;
    }

    g_Camera.type = CAM_BINOCULARS;
    M_HandleLookInput();
    M_HandleZoomInput();
    if (!g_Input.action) {
        m_TorchHeldOff = false;
    }
    m_TorchActive = g_Input.action && !m_TorchHeldOff;

    g_Input = (INPUT_STATE) {};
    g_InputDB = (INPUT_STATE) {};
}

void Camera_Binoculars_Update(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    lara_item->mesh_bits = 0;
    Viewport_AlterFOV(7 * (2080 - m_Range), m_OriginalFOVMode);

    int16_t pitch = lara_info->head_rot.x << 1;
    CLAMP(pitch, -M_MAX_PITCH, M_MAX_PITCH);
    int16_t yaw = lara_info->head_rot.y;
    CLAMP(yaw, -M_MAX_YAW, M_MAX_YAW);
    yaw += lara_item->rot.y;

    XYZ_32 eye = lara_item->pos;
    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(eye, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, eye);
    if (ceiling <= eye.y - (WALL_L * 3) / 4) {
        eye.y -= (WALL_L * 3) / 4;
    } else {
        eye.y += STEP_L / 4;
    }

    // Avoid clipping into low ceilings either directly above or just ahead.
    GAME_VECTOR clamp_pos = {
        .pos = XYZ_32_OffsetYaw(eye, yaw, STEP_L / 4),
        .room_num = room_num,
    };
    Camera_Collide(&clamp_pos, STEP_L / 4, true);
    eye.y = clamp_pos.y;

    const XYZ_32 target =
        XYZ_32_Add(eye, XYZ_32_FromYawPitch(yaw, pitch, M_TARGET_DIST));

    g_Camera.pos.pos = eye;
    g_Camera.pos.room_num = room_num;

    if (m_JustEntered || m_OriginalCamera.type == CAM_FIXED) {
        g_Camera.target.pos = target;
    } else {
        g_Camera.target.x += (target.x - g_Camera.target.x) >> 2;
        g_Camera.target.y += (target.y - g_Camera.target.y) >> 2;
        g_Camera.target.z += (target.z - g_Camera.target.z) >> 2;
    }
    g_Camera.target.room_num = lara_item->room_num;
    m_JustEntered = false;

    Camera_ApplyBounce();
    Room_GetSector(g_Camera.pos.pos, &g_Camera.pos.room_num);
    g_Camera.mic_pos = g_Camera.pos;

    if (m_TorchActive) {
        M_EmitTorch(g_Camera.pos.pos, g_Camera.target.pos);
    }
    if (m_LaserSight) {
        GAME_VECTOR sight = g_Camera.target;
        sight.room_num = lara_item->room_num;
        Gun_LaserSight_Aim(
            lara_info->gun_type, g_Camera.pos, sight, m_FirePending);
        m_FirePending = false;
    }
}

void Camera_Binoculars_Exit(void)
{
    if (!m_Active) {
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();

    m_Active = false;
    m_Pending = false;
    m_TorchActive = false;
    m_TorchHeldOff = false;
    lara_item->mesh_bits = 0xFFFFFFFF;
    // The laser sight leaves the weapon ready in Lara's hands.
    if (m_LaserSight) {
        m_LaserSight = false;
        m_FirePending = false;
        FX_LaserSight_ClearDot();
    } else {
        lara_info->gun_status = LGS_ARMLESS;
    }
    lara_info->head_rot.x = 0;
    lara_info->head_rot.y = 0;
    lara_info->torso_rot.x = 0;
    lara_info->torso_rot.y = 0;
    Viewport_AlterFOV(m_OriginalFOV, m_OriginalFOVMode);

    // Lara cannot move while the binoculars are up, so the camera state
    // saved on entering is still valid — restore it wholesale to return
    // exactly to the pre-binocular view.
    const bool underwater = g_Camera.underwater;
    g_Camera = m_OriginalCamera;
    g_Camera.underwater = underwater;
    if (g_Camera.type == CAM_BINOCULARS) {
        g_Camera.type = CAM_CHASE;
    }
}

bool Camera_Binoculars_IsActive(void)
{
    return m_Active;
}

bool Camera_Binoculars_IsLaserSight(void)
{
    return m_Active && m_LaserSight;
}

int32_t Camera_Binoculars_GetRange(void)
{
    return m_Range;
}

#include "game/lara/look.h"

#include "config.h"
#include "game/camera.h"
#include "game/input.h"
#include "game/lara.h"
#include "utils.h"

// clang-format off
#if TR_VERSION == 1
    #define M_MAX_HEAD_ROTATION      (50 * DEG_1)            // = 9100
#else
    #define M_MAX_HEAD_ROTATION      (44 * DEG_1)            // = 8008
#endif
#define M_HEAD_TURN                  (2 * DEG_1)             // = 364
#define M_MIN_HEAD_ROTATION          (-M_MAX_HEAD_ROTATION)  // = -8008
#define M_MAX_HEAD_TILT              (22 * DEG_1)            // = 4004
#define M_MIN_HEAD_TILT              (-42 * DEG_1)           // = -7644
#define M_HEAD_TURN_SURF             (3 * DEG_1)             // = 546
#define M_MAX_HEAD_ROTATION_SURF     (50 * DEG_1)            // = 9100
#define M_MAX_HEAD_TILT_SURF         (40 * DEG_1)            // = 7280
#define M_MIN_HEAD_TILT_SURF         (-M_MAX_HEAD_TILT_SURF) // = -7280
// clang-format on

static const LARA_STATE m_StopStates[] = {
    LS_STOP,
    LS_SURF_TREAD,
    (LARA_STATE)-1,
};

static void M_Reset(void);

static void M_Reset(void)
{
    if (g_Camera.type == CAM_LOOK) {
        return;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->head_rot.x <= -M_HEAD_TURN || lara->head_rot.x >= M_HEAD_TURN) {
        lara->head_rot.x -= lara->head_rot.x / 8;
    } else {
        lara->head_rot.x = 0;
    }

    if (lara->head_rot.y <= -M_HEAD_TURN || lara->head_rot.y >= M_HEAD_TURN) {
        lara->head_rot.y += lara->head_rot.y / -8;
    } else {
        lara->head_rot.y = 0;
    }

    lara->torso_rot.x = lara->head_rot.x;
    lara->torso_rot.y = lara->head_rot.y;
}

void Lara_Look_LeftRight(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    g_Camera.type = CAM_LOOK;

    const bool on_surface = lara->water_status == LWS_SURFACE;
    const int16_t max_head_rot =
        on_surface ? M_MAX_HEAD_ROTATION_SURF : M_MAX_HEAD_ROTATION;
    const int16_t head_turn = on_surface ? M_HEAD_TURN_SURF : M_HEAD_TURN;

    if (g_Input.left) {
        g_Input.left = 0;
        if (lara->head_rot.y > -max_head_rot) {
            lara->head_rot.y -= head_turn;
        }
    } else if (g_Input.right) {
        g_Input.right = 0;
        if (lara->head_rot.y < max_head_rot) {
            lara->head_rot.y += head_turn;
        }
    }

#if TR_VERSION == 1
    const bool on_vehicle = false;
#else
    const bool on_vehicle = lara->vehicle_item_num != NO_ITEM;
#endif
    if (lara->gun_status != LGS_HANDS_BUSY && !on_vehicle) {
        lara->torso_rot.y =
            on_surface ? (lara->head_rot.y / 2) : lara->head_rot.y;
    }
}

void Lara_Look_UpDown(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    g_Camera.type = CAM_LOOK;
#if TR_VERSION == 1
    if (g_Config.gameplay.enable_inverted_look) {
        bool temp_forward;
        SWAP(g_Input.forward, g_Input.back, temp_forward);
    }
#endif

    const bool on_surface = lara->water_status == LWS_SURFACE;
    const int16_t min_head_tilt =
        on_surface ? M_MIN_HEAD_TILT_SURF : M_MIN_HEAD_TILT;
    const int16_t max_head_tilt =
        on_surface ? M_MAX_HEAD_TILT_SURF : M_MAX_HEAD_TILT;
    const int16_t head_turn = on_surface ? M_HEAD_TURN_SURF : M_HEAD_TURN;

    if (g_Input.forward) {
        g_Input.forward = 0;
        if (lara->head_rot.x > min_head_tilt) {
            lara->head_rot.x -= head_turn;
        }
    } else if (g_Input.back) {
        g_Input.back = 0;
        if (lara->head_rot.x < max_head_tilt) {
            lara->head_rot.x += head_turn;
        }
    }

    if (lara->gun_status != LGS_HANDS_BUSY) {
        lara->torso_rot.x = on_surface ? 0 : lara->head_rot.x;
    }
}

void Lara_Look_Update(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();
    if (g_Input.look && !g_Config.gameplay.enable_enhanced_look
        && !Lara_HasState(m_StopStates)) {
        g_Camera.type = CAM_CHASE;
        M_Reset();
        return;
    }

#if TR_VERSION == 1
    const bool can_look = lara_item->hit_points > 0;
#else
    const bool can_look = lara_info->enable_look;
#endif
    if (g_Input.look && !lara_info->extra_anim && can_look) {
        Lara_Look_LeftRight();
    } else {
        M_Reset();
    }

#if TR_VERSION >= 2
    lara_info->enable_look = true;
#endif
}

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/input/backends/base.h>
#include <trx/game/input/backends/controller.h>
#include <trx/game/input/backends/keyboard.h>
#include <trx/game/input/backends/touch.h>
#include <trx/game/input/common.h>
#include <trx/game/lara.h>
#include <trx/version.h>

// How long TR4 withholds the look input before it counts as a look rather
// than a tap, in frames.
#define M_TR4_LOOK_DELAY 6

static int32_t m_LookFrames = 0;
static bool m_IsLookHeld = false;

static void M_UpdateFromBackend(
    INPUT_STATE *const s, const INPUT_BACKEND_IMPL *const backend,
    const int32_t layout)
{
#define X_INPUT_ROLE(role, state) s->state |= backend->is_pressed(layout, role);
#include <trx/game/input/roles.def>
#undef X_INPUT_ROLE
    backend->custom_update(s, layout);
}

// TR4 changes target off the look input rather than one of its own. With
// weapons ready it withholds look for the first few frames; letting go before
// they are up counts as a tap, and changes target instead.
static void M_UpdateTargetChange(void)
{
    const TARGET_CHANGE_MODE mode = g_Config.gameplay.target_change_mode;
    const bool is_ready = Lara_GetLaraInfo()->gun_status == LGS_READY;

    if (mode != TARGET_CHANGE_MODE_TR4 || !is_ready) {
        m_LookFrames = 0;
        m_IsLookHeld = false;
        if (mode == TARGET_CHANGE_MODE_OFF || !is_ready) {
            g_Input.change_target = 0;
        }
        return;
    }

    // Look drives it here, so a binding of its own does not.
    g_Input.change_target = 0;

    if (!g_Input.look) {
        g_Input.change_target = m_LookFrames > 0 && !m_IsLookHeld;
        m_LookFrames = 0;
        m_IsLookHeld = false;
        return;
    }

    if (!m_IsLookHeld && ++m_LookFrames > M_TR4_LOOK_DELAY) {
        m_IsLookHeld = true;
    }
    g_Input.look = m_IsLookHeld;
}

void Input_Update(void)
{
    InputState_Clear(&g_Input);

    M_UpdateFromBackend(
        &g_Input, &g_Input_Keyboard,
        g_Config.input.layout[INPUT_BACKEND_KEYBOARD]);
    M_UpdateFromBackend(
        &g_Input, &g_Input_Controller,
        g_Config.input.layout[INPUT_BACKEND_CONTROLLER]);
    M_UpdateFromBackend(
        &g_Input, &g_Input_Touch, g_Config.input.layout[INPUT_BACKEND_TOUCH]);

    // Suppress roles whose bindings are subsets of longer active combos.
    g_Input_Keyboard.resolve_combos(
        g_Config.input.layout[INPUT_BACKEND_KEYBOARD], &g_Input);
    g_Input_Controller.resolve_combos(
        g_Config.input.layout[INPUT_BACKEND_CONTROLLER], &g_Input);
    g_Input_Touch.resolve_combos(
        g_Config.input.layout[INPUT_BACKEND_TOUCH], &g_Input);

    g_Input.camera_reset |= g_Input.look;
    g_Input.menu_up |= g_Input.forward;
    g_Input.menu_down |= g_Input.back;
    g_Input.menu_left |= g_Input.left;
    g_Input.menu_right |= g_Input.right;
    g_Input.menu_back |= g_Input.option;
    g_Input.option &= g_Camera.type != CAM_CINEMATIC;
    g_Input.roll |= g_Input.forward && g_Input.back;
    if (g_Input.left && g_Input.right) {
        g_Input.left = 0;
        g_Input.right = 0;
    }

    if (!g_Config.gameplay.enable_crawling && g_Camera.type != CAM_BINOCULARS) {
        g_Input.crouch = 0;
    }

    if (g_Config.input.enable_tr3_sidesteps) {
        if (g_Input.slow && !g_Input.forward && !g_Input.back
            && !g_Input.step_left && !g_Input.step_right) {
            if (g_Input.left) {
                g_Input.left = 0;
                g_Input.step_left = 1;
            } else if (g_Input.right) {
                g_Input.right = 0;
                g_Input.step_right = 1;
            }
        }
    }

    M_UpdateTargetChange();

    g_InputDB = Input_GetDebounced(g_Input);

    if (Input_IsInListenMode()) {
        InputState_Clear(&g_Input);
        InputState_Clear(&g_InputDB);
    }
}

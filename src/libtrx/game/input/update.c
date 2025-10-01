#include "config.h"
#include "game/camera.h"
#include "game/game_string.h"
#include "game/input/backends/base.h"
#include "game/input/backends/controller.h"
#include "game/input/backends/keyboard.h"
#include "game/input/common.h"
#include "game/lara.h"

static void M_UpdateFromBackend(
    INPUT_STATE *const s, const INPUT_BACKEND_IMPL *const backend,
    const int32_t layout)
{
#define X_INPUT_ROLE(role_name, state_name)                                    \
    s->state_name |= backend->is_pressed(layout, role_name);
#include "game/input/roles.def"
#undef X_INPUT_ROLE
    backend->custom_update(s, layout);
}

void Input_Update(void)
{
    g_Input.any = 0;

    M_UpdateFromBackend(
        &g_Input, &g_Input_Keyboard, g_Config.input.keyboard_layout);
    M_UpdateFromBackend(
        &g_Input, &g_Input_Controller, g_Config.input.controller_layout);

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

    if (!g_Config.gameplay.enable_target_change
        || Lara_GetLaraInfo()->gun_status != LGS_READY) {
        g_Input.change_target = 0;
    }

    g_InputDB = Input_GetDebounced(g_Input);

    if (Input_IsInListenMode()) {
        g_Input.any = 0;
        g_InputDB.any = 0;
    }
}

#include "game/input.h"

#include "game/camera.h"
#include "game/clock.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/input/backends/base.h>
#include <libtrx/game/input/backends/controller.h>
#include <libtrx/game/input/backends/keyboard.h>

static void M_UpdateFromBackend(
    INPUT_STATE *s, const INPUT_BACKEND_IMPL *backend, int32_t layout);

static void M_UpdateFromBackend(
    INPUT_STATE *const s, const INPUT_BACKEND_IMPL *const backend,
    const int32_t layout)
{
    // clang-format off
    s->forward                   |= backend->is_pressed(layout, INPUT_ROLE_UP);
    s->back                      |= backend->is_pressed(layout, INPUT_ROLE_DOWN);
    s->left                      |= backend->is_pressed(layout, INPUT_ROLE_LEFT);
    s->right                     |= backend->is_pressed(layout, INPUT_ROLE_RIGHT);
    s->step_left                 |= backend->is_pressed(layout, INPUT_ROLE_STEP_L);
    s->step_right                |= backend->is_pressed(layout, INPUT_ROLE_STEP_R);
    s->slow                      |= backend->is_pressed(layout, INPUT_ROLE_SLOW);
    s->jump                      |= backend->is_pressed(layout, INPUT_ROLE_JUMP);
    s->action                    |= backend->is_pressed(layout, INPUT_ROLE_ACTION);
    s->draw                      |= backend->is_pressed(layout, INPUT_ROLE_DRAW);
    s->look                      |= backend->is_pressed(layout, INPUT_ROLE_LOOK);
    s->roll                      |= backend->is_pressed(layout, INPUT_ROLE_ROLL);

    s->enter_console             |= backend->is_pressed(layout, INPUT_ROLE_ENTER_CONSOLE);
    s->save                      |= backend->is_pressed(layout, INPUT_ROLE_SAVE);
    s->load                      |= backend->is_pressed(layout, INPUT_ROLE_LOAD);

    s->pause                     |= backend->is_pressed(layout, INPUT_ROLE_PAUSE);
    s->toggle_ui                 |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_UI);
    s->toggle_photo_mode         |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_PHOTO_MODE);

    s->camera_up                 |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_UP);
    s->camera_down               |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_DOWN);
    s->camera_forward            |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_FORWARD);
    s->camera_back               |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_BACK);
    s->camera_left               |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_LEFT);
    s->camera_right              |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_RIGHT);
    s->change_target             |= backend->is_pressed(layout, INPUT_ROLE_CHANGE_TARGET);

    s->item_cheat                |= backend->is_pressed(layout, INPUT_ROLE_ITEM_CHEAT);
    s->fly_cheat                 |= backend->is_pressed(layout, INPUT_ROLE_FLY_CHEAT);
    s->level_skip_cheat          |= backend->is_pressed(layout, INPUT_ROLE_LEVEL_SKIP_CHEAT);
    s->turbo_cheat               |= backend->is_pressed(layout, INPUT_ROLE_TURBO_CHEAT);

    s->equip_pistols             |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_PISTOLS);
    s->equip_shotgun             |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_SHOTGUN);
    s->equip_magnums             |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_MAGNUMS);
    s->equip_uzis                |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_UZIS);
    s->use_small_medi            |= backend->is_pressed(layout, INPUT_ROLE_USE_SMALL_MEDI);
    s->use_big_medi              |= backend->is_pressed(layout, INPUT_ROLE_USE_BIG_MEDI);

    s->option                    |= backend->is_pressed(layout, INPUT_ROLE_OPTION);
    s->menu_up                   |= backend->is_pressed(layout, INPUT_ROLE_MENU_UP);
    s->menu_down                 |= backend->is_pressed(layout, INPUT_ROLE_MENU_DOWN);
    s->menu_left                 |= backend->is_pressed(layout, INPUT_ROLE_MENU_LEFT);
    s->menu_right                |= backend->is_pressed(layout, INPUT_ROLE_MENU_RIGHT);
    s->menu_confirm              |= backend->is_pressed(layout, INPUT_ROLE_MENU_CONFIRM);
    s->menu_back                 |= backend->is_pressed(layout, INPUT_ROLE_MENU_BACK);

    s->screenshot                |= backend->is_pressed(layout, INPUT_ROLE_SCREENSHOT);
    s->toggle_fps_counter        |= backend->is_pressed(layout, INPUT_ROLE_FPS);
    s->toggle_bilinear_filter    |= backend->is_pressed(layout, INPUT_ROLE_BILINEAR);
    s->toggle_trapezoid_filter   |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER);
    // clang-format on

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

    if (!g_Config.gameplay.enable_cheats) {
        g_Input.item_cheat = 0;
        g_Input.fly_cheat = 0;
        g_Input.level_skip_cheat = 0;
        g_Input.turbo_cheat = 0;
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
        || g_Lara.gun_status != LGS_READY) {
        g_Input.change_target = 0;
    }

    g_InputDB = Input_GetDebounced(g_Input);

    if (Input_IsInListenMode()) {
        g_Input.any = 0;
        g_InputDB.any = 0;
    }
}

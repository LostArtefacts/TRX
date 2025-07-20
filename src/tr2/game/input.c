#include "game/input.h"

#include "game/console/common.h"
#include "game/game_string.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/input/backends/base.h>
#include <libtrx/game/input/backends/controller.h>
#include <libtrx/game/input/backends/keyboard.h>

static void M_UpdateFromBackend(
    INPUT_STATE *s, const INPUT_BACKEND_IMPL *backend, INPUT_LAYOUT layout);

static void M_UpdateFromBackend(
    INPUT_STATE *const s, const INPUT_BACKEND_IMPL *const backend,
    const INPUT_LAYOUT layout)
{
    // clang-format off
    s->forward                     |= backend->is_pressed(layout, INPUT_ROLE_UP);
    s->back                        |= backend->is_pressed(layout, INPUT_ROLE_DOWN);
    s->left                        |= backend->is_pressed(layout, INPUT_ROLE_LEFT);
    s->right                       |= backend->is_pressed(layout, INPUT_ROLE_RIGHT);
    s->step_left                   |= backend->is_pressed(layout, INPUT_ROLE_STEP_LEFT);
    s->step_right                  |= backend->is_pressed(layout, INPUT_ROLE_STEP_RIGHT);
    s->slow                        |= backend->is_pressed(layout, INPUT_ROLE_SLOW);
    s->jump                        |= backend->is_pressed(layout, INPUT_ROLE_JUMP);
    s->action                      |= backend->is_pressed(layout, INPUT_ROLE_ACTION);
    s->draw                        |= backend->is_pressed(layout, INPUT_ROLE_DRAW_WEAPON);
    s->look                        |= backend->is_pressed(layout, INPUT_ROLE_LOOK);
    s->roll                        |= backend->is_pressed(layout, INPUT_ROLE_ROLL);

    s->enter_console               |= backend->is_pressed(layout, INPUT_ROLE_ENTER_CONSOLE);
    s->save                        |= backend->is_pressed(layout, INPUT_ROLE_SAVE);
    s->load                        |= backend->is_pressed(layout, INPUT_ROLE_LOAD);

    s->pause                       |= backend->is_pressed(layout, INPUT_ROLE_PAUSE);
    s->toggle_ui                   |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_UI);
    s->toggle_photo_mode           |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_PHOTO_MODE);

    s->camera_up                   |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_UP);
    s->camera_down                 |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_DOWN);
    s->camera_forward              |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_FORWARD);
    s->camera_back                 |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_BACK);
    s->camera_left                 |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_LEFT);
    s->camera_right                |= backend->is_pressed(layout, INPUT_ROLE_CAMERA_RIGHT);

    s->item_cheat                  |= backend->is_pressed(layout, INPUT_ROLE_ITEM_CHEAT);
    s->fly_cheat                   |= backend->is_pressed(layout, INPUT_ROLE_FLY_CHEAT);
    s->level_skip_cheat            |= backend->is_pressed(layout, INPUT_ROLE_LEVEL_SKIP_CHEAT);
    s->turbo_cheat                 |= backend->is_pressed(layout, INPUT_ROLE_TURBO_CHEAT);

    s->equip_pistols               |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_PISTOLS);
    s->equip_shotgun               |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_SHOTGUN);
    s->equip_magnums               |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_MAGNUMS);
    s->equip_uzis                  |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_UZIS);
    s->equip_harpoon               |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_HARPOON);
    s->equip_m16                   |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_M16);
    s->equip_grenade_launcher      |= backend->is_pressed(layout, INPUT_ROLE_EQUIP_GRENADE_LAUNCHER);
    s->use_flare                   |= backend->is_pressed(layout, INPUT_ROLE_USE_FLARE);
    s->use_small_medi              |= backend->is_pressed(layout, INPUT_ROLE_USE_SMALL_MEDI);
    s->use_big_medi                |= backend->is_pressed(layout, INPUT_ROLE_USE_BIG_MEDI);

    s->option                      |= backend->is_pressed(layout, INPUT_ROLE_INVENTORY);
    s->menu_up                     |= backend->is_pressed(layout, INPUT_ROLE_MENU_UP);
    s->menu_down                   |= backend->is_pressed(layout, INPUT_ROLE_MENU_DOWN);
    s->menu_left                   |= backend->is_pressed(layout, INPUT_ROLE_MENU_LEFT);
    s->menu_right                  |= backend->is_pressed(layout, INPUT_ROLE_MENU_RIGHT);
    s->menu_confirm                |= backend->is_pressed(layout, INPUT_ROLE_MENU_CONFIRM);
    s->menu_back                   |= backend->is_pressed(layout, INPUT_ROLE_MENU_BACK);
    s->reset_bindings              |= backend->is_pressed(layout, INPUT_ROLE_RESET_BINDINGS);
    s->unbind_key                  |= backend->is_pressed(layout, INPUT_ROLE_UNBIND_KEY);

    s->screenshot                  |= backend->is_pressed(layout, INPUT_ROLE_SCREENSHOT);
    s->switch_upscaling            |= backend->is_pressed(layout, INPUT_ROLE_SWITCH_UPSCALING);
    s->switch_borders              |= backend->is_pressed(layout, INPUT_ROLE_SWITCH_BORDERS);
    s->toggle_fps_counter          |= backend->is_pressed(layout, INPUT_ROLE_FPS);
    s->toggle_bilinear_filter      |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_BILINEAR_FILTER);
    s->toggle_perspective_filter   |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_PERSPECTIVE_FILTER);
    s->toggle_trapezoid_filter     |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER);
    s->toggle_z_buffer             |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_Z_BUFFER);
    s->cycle_lighting_contrast     |= backend->is_pressed(layout, INPUT_ROLE_CYCLE_LIGHTING_CONTRAST);
    s->toggle_fullscreen           |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_FULLSCREEN);
    s->toggle_rendering_mode       |= backend->is_pressed(layout, INPUT_ROLE_TOGGLE_RENDERING_MODE);
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

    g_InputDB = Input_GetDebounced(g_Input);

    if (Input_IsInListenMode()) {
        g_Input = (INPUT_STATE) {};
        g_InputDB = (INPUT_STATE) {};
    }
}

const char *Input_GetRoleName(const INPUT_ROLE role)
{
    // clang-format off
    switch (role) {
    case INPUT_ROLE_UP:                          return GS(ENUM_INPUT_ROLE_UP);
    case INPUT_ROLE_DOWN:                        return GS(ENUM_INPUT_ROLE_DOWN);
    case INPUT_ROLE_LEFT:                        return GS(ENUM_INPUT_ROLE_LEFT);
    case INPUT_ROLE_RIGHT:                       return GS(ENUM_INPUT_ROLE_RIGHT);
    case INPUT_ROLE_STEP_LEFT:                   return GS(ENUM_INPUT_ROLE_STEP_LEFT);
    case INPUT_ROLE_STEP_RIGHT:                  return GS(ENUM_INPUT_ROLE_STEP_RIGHT);
    case INPUT_ROLE_SLOW:                        return GS(ENUM_INPUT_ROLE_SLOW);
    case INPUT_ROLE_JUMP:                        return GS(ENUM_INPUT_ROLE_JUMP);
    case INPUT_ROLE_ACTION:                      return GS(ENUM_INPUT_ROLE_ACTION);
    case INPUT_ROLE_DRAW_WEAPON:                 return GS(ENUM_INPUT_ROLE_DRAW_WEAPON);
    case INPUT_ROLE_EQUIP_PISTOLS:               return GS(ENUM_INPUT_ROLE_EQUIP_PISTOLS);
    case INPUT_ROLE_EQUIP_SHOTGUN:               return GS(ENUM_INPUT_ROLE_EQUIP_SHOTGUN);
    case INPUT_ROLE_EQUIP_MAGNUMS:               return GS(ENUM_INPUT_ROLE_EQUIP_MAGNUMS);
    case INPUT_ROLE_EQUIP_UZIS:                  return GS(ENUM_INPUT_ROLE_EQUIP_UZIS);
    case INPUT_ROLE_EQUIP_HARPOON:               return GS(ENUM_INPUT_ROLE_EQUIP_HARPOON);
    case INPUT_ROLE_EQUIP_M16:                   return GS(ENUM_INPUT_ROLE_EQUIP_M16);
    case INPUT_ROLE_EQUIP_GRENADE_LAUNCHER:      return GS(ENUM_INPUT_ROLE_EQUIP_GRENADE_LAUNCHER);
    case INPUT_ROLE_USE_SMALL_MEDI:              return GS(ENUM_INPUT_ROLE_USE_SMALL_MEDI);
    case INPUT_ROLE_USE_BIG_MEDI:                return GS(ENUM_INPUT_ROLE_USE_BIG_MEDI);
    case INPUT_ROLE_USE_FLARE:                   return GS(ENUM_INPUT_ROLE_USE_FLARE);
    case INPUT_ROLE_LOOK:                        return GS(ENUM_INPUT_ROLE_LOOK);
    case INPUT_ROLE_ROLL:                        return GS(ENUM_INPUT_ROLE_ROLL);
    case INPUT_ROLE_INVENTORY:                   return GS(ENUM_INPUT_ROLE_INVENTORY);
    case INPUT_ROLE_FLY_CHEAT:                   return GS(ENUM_INPUT_ROLE_FLY_CHEAT);
    case INPUT_ROLE_ITEM_CHEAT:                  return GS(ENUM_INPUT_ROLE_ITEM_CHEAT);
    case INPUT_ROLE_LEVEL_SKIP_CHEAT:            return GS(ENUM_INPUT_ROLE_LEVEL_SKIP_CHEAT);
    case INPUT_ROLE_TURBO_CHEAT:                 return GS(ENUM_INPUT_ROLE_TURBO_CHEAT);
    case INPUT_ROLE_ENTER_CONSOLE:               return GS(ENUM_INPUT_ROLE_ENTER_CONSOLE);
    case INPUT_ROLE_PAUSE:                       return GS(ENUM_INPUT_ROLE_PAUSE);
    case INPUT_ROLE_TOGGLE_UI:                   return GS(ENUM_INPUT_ROLE_TOGGLE_UI);
    case INPUT_ROLE_TOGGLE_PHOTO_MODE:           return GS(ENUM_INPUT_ROLE_TOGGLE_PHOTO_MODE);
    case INPUT_ROLE_CAMERA_RESET:                return GS(ENUM_INPUT_ROLE_CAMERA_RESET);
    case INPUT_ROLE_CAMERA_UP:                   return GS(ENUM_INPUT_ROLE_CAMERA_UP);
    case INPUT_ROLE_CAMERA_DOWN:                 return GS(ENUM_INPUT_ROLE_CAMERA_DOWN);
    case INPUT_ROLE_CAMERA_FORWARD:              return GS(ENUM_INPUT_ROLE_CAMERA_FORWARD);
    case INPUT_ROLE_CAMERA_BACK:                 return GS(ENUM_INPUT_ROLE_CAMERA_BACK);
    case INPUT_ROLE_CAMERA_LEFT:                 return GS(ENUM_INPUT_ROLE_CAMERA_LEFT);
    case INPUT_ROLE_CAMERA_RIGHT:                return GS(ENUM_INPUT_ROLE_CAMERA_RIGHT);
    case INPUT_ROLE_SAVE:                        return GS(ENUM_INPUT_ROLE_SAVE);
    case INPUT_ROLE_LOAD:                        return GS(ENUM_INPUT_ROLE_LOAD);
    case INPUT_ROLE_SCREENSHOT:                  return GS(ENUM_INPUT_ROLE_SCREENSHOT);
    case INPUT_ROLE_FPS:                         return GS(ENUM_INPUT_ROLE_FPS);
    case INPUT_ROLE_TOGGLE_FULLSCREEN:           return GS(ENUM_INPUT_ROLE_TOGGLE_FULLSCREEN);
    case INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER:     return GS(ENUM_INPUT_ROLE_TOGGLE_TRAPEZOID_FILTER);
    case INPUT_ROLE_SWITCH_UPSCALING:            return GS(ENUM_INPUT_ROLE_SWITCH_UPSCALING);
    case INPUT_ROLE_SWITCH_BORDERS:              return GS(ENUM_INPUT_ROLE_SWITCH_BORDERS);
    case INPUT_ROLE_TOGGLE_BILINEAR_FILTER:      return GS(ENUM_INPUT_ROLE_TOGGLE_BILINEAR_FILTER);
    case INPUT_ROLE_TOGGLE_PERSPECTIVE_FILTER:   return GS(ENUM_INPUT_ROLE_TOGGLE_PERSPECTIVE_FILTER);
    case INPUT_ROLE_TOGGLE_Z_BUFFER:             return GS(ENUM_INPUT_ROLE_TOGGLE_Z_BUFFER);
    case INPUT_ROLE_CYCLE_LIGHTING_CONTRAST:     return GS(ENUM_INPUT_ROLE_CYCLE_LIGHTING_CONTRAST);
    case INPUT_ROLE_TOGGLE_RENDERING_MODE:       return GS(ENUM_INPUT_ROLE_TOGGLE_RENDERING_MODE);
    default:                                     return "";
    }
    // clang-format on
}

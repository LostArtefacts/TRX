#include "specific/s_input.h"

#include "decomp/decomp.h"
#include "game/console/common.h"
#include "game/gameflow/gameflow_new.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/lara/control.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <dinput.h>

#define KEY_DOWN(key) ((g_DIKeys[(key)] & 0x80))

INPUT_LAYOUT g_Layout[2] = {
    { .key = {
        DIK_UP,
        DIK_DOWN,
        DIK_LEFT,
        DIK_RIGHT,
        DIK_DELETE,
        DIK_NEXT,
        DIK_RSHIFT,
        DIK_RMENU,
        DIK_RCONTROL,
        DIK_SPACE,
        DIK_PERIOD,
        DIK_NUMPAD0,
        DIK_END,
        DIK_ESCAPE,
        DIK_SLASH,
    } },
    { .key = {
        DIK_NUMPAD8,
        DIK_NUMPAD2,
        DIK_NUMPAD4,
        DIK_NUMPAD6,
        DIK_NUMPAD7,
        DIK_NUMPAD9,
        DIK_NUMPAD1,
        DIK_ADD,
        DIK_NUMPADENTER,
        DIK_NUMPAD3,
        DIK_SUBTRACT,
        DIK_NUMPAD0,
        DIK_NUMPAD5,
        DIK_DECIMAL,
        DIK_SLASH,
    } }
};

static bool m_IsF3Pressed = false;
static bool m_IsF4Pressed = false;
static bool m_IsF7Pressed = false;
static bool m_IsF8Pressed = false;
static bool m_IsF11Pressed = false;
static int32_t m_MediPackCooldown = 0;

static bool M_KbdKey(int32_t layout_num, INPUT_ROLE role);
static bool M_JoyKey(int32_t layout_num, INPUT_ROLE role);

static bool M_KbdKey(const int32_t layout_num, const INPUT_ROLE role)
{
    uint16_t key = Input_GetAssignedKey(layout_num, role);
    if (key >= 0x100) {
        return false;
    }

    if (KEY_DOWN(key)) {
        return true;
    }
    switch (key) {
    case DIK_RCONTROL:
        return KEY_DOWN(DIK_LCONTROL);
    case DIK_LCONTROL:
        return KEY_DOWN(DIK_RCONTROL);
    case DIK_RSHIFT:
        return KEY_DOWN(DIK_LSHIFT);
    case DIK_LSHIFT:
        return KEY_DOWN(DIK_RSHIFT);
    case DIK_RMENU:
        return KEY_DOWN(DIK_LMENU);
    case DIK_LMENU:
        return KEY_DOWN(DIK_RMENU);
    }
    return false;
}

static bool M_JoyKey(const int32_t layout_num, const INPUT_ROLE role)
{
    uint16_t key = Input_GetAssignedKey(layout_num, role);
    if (key < 0x100) {
        return false;
    }
    return (g_JoyKeys & (1 << key)) != 0;
}

bool __cdecl S_Input_Key(const INPUT_ROLE role)
{
    return M_KbdKey(1, role) || M_JoyKey(1, role)
        || (!g_ConflictLayout[role] && M_KbdKey(0, role));
}

// TODO: refactor me!!!
bool __cdecl S_Input_Update(void)
{
    WinVidSpinMessageLoop(0);
    WinInReadKeyboard(g_DIKeys);

    int32_t joy_xpos = 0;
    int32_t joy_ypos = 0;
    g_JoyKeys = WinInReadJoystick(&joy_xpos, &joy_ypos);

    INPUT_STATE input = 0;
    if (joy_xpos < -8) {
        input |= IN_LEFT;
    } else if (joy_xpos > 8) {
        input |= IN_RIGHT;
    }

    if (joy_ypos > 8) {
        input |= IN_BACK;
    } else if (joy_ypos < -8) {
        input |= IN_FORWARD;
    }

    if (S_Input_Key(INPUT_ROLE_FORWARD)) {
        input |= IN_FORWARD;
    }
    if (S_Input_Key(INPUT_ROLE_BACK)) {
        input |= IN_BACK;
    }
    if (S_Input_Key(INPUT_ROLE_LEFT)) {
        input |= IN_LEFT;
    }
    if (S_Input_Key(INPUT_ROLE_RIGHT)) {
        input |= IN_RIGHT;
    }
    if (S_Input_Key(INPUT_ROLE_STEP_LEFT)) {
        input |= IN_STEP_LEFT;
    }
    if (S_Input_Key(INPUT_ROLE_STEP_RIGHT)) {
        input |= IN_STEP_RIGHT;
    }
    if (S_Input_Key(INPUT_ROLE_SLOW)) {
        input |= IN_SLOW;
    }
    if (S_Input_Key(INPUT_ROLE_JUMP)) {
        input |= IN_JUMP;
    }

    if (S_Input_Key(INPUT_ROLE_ACTION)) {
        input |= IN_ACTION;
    }
    if (S_Input_Key(INPUT_ROLE_DRAW_WEAPON)) {
        input |= IN_DRAW;
    }
    if (S_Input_Key(INPUT_ROLE_FLARE)) {
        input |= IN_FLARE;
    }
    if (S_Input_Key(INPUT_ROLE_LOOK)) {
        input |= IN_LOOK;
    }
    if (S_Input_Key(INPUT_ROLE_ROLL)) {
        input |= IN_ROLL;
    }

    if (S_Input_Key(INPUT_ROLE_CONSOLE)) {
        input |= IN_CONSOLE;
    }

    if (S_Input_Key(INPUT_ROLE_OPTION) && g_Camera.type != CAM_CINEMATIC) {
        input |= IN_OPTION;
    }
    if ((input & IN_FORWARD) && (input & IN_BACK)) {
        input |= IN_ROLL;
    }

    if (KEY_DOWN(DIK_RETURN) || (input & IN_ACTION)) {
        input |= IN_SELECT;
    }
    if (KEY_DOWN(DIK_ESCAPE)) {
        input |= IN_DESELECT;
    }

    if ((input & IN_LEFT) && (input & IN_RIGHT)) {
        input &= ~IN_LEFT;
        input &= ~IN_RIGHT;
    }

    if (g_IsFMVPlaying || Console_IsOpened()) {
        g_Input = input;
        return g_IsGameToExit;
    }

    if (g_GameInfo.current_level.type != GFL_DEMO) {
        if (KEY_DOWN(DIK_1) && Inv_RequestItem(O_PISTOL_OPTION)) {
            g_Lara.request_gun_type = LGT_PISTOLS;
        } else if (KEY_DOWN(DIK_2) && Inv_RequestItem(O_SHOTGUN_OPTION)) {
            g_Lara.request_gun_type = LGT_SHOTGUN;
        } else if (KEY_DOWN(DIK_3) && Inv_RequestItem(O_MAGNUM_OPTION)) {
            g_Lara.request_gun_type = LGT_MAGNUMS;
        } else if (KEY_DOWN(DIK_4) && Inv_RequestItem(O_UZI_OPTION)) {
            g_Lara.request_gun_type = LGT_UZIS;
        } else if (KEY_DOWN(DIK_5) && Inv_RequestItem(O_HARPOON_OPTION)) {
            g_Lara.request_gun_type = LGT_HARPOON;
        } else if (KEY_DOWN(DIK_6) && Inv_RequestItem(O_M16_OPTION)) {
            g_Lara.request_gun_type = LGT_M16;
        } else if (KEY_DOWN(DIK_7) && Inv_RequestItem(O_GRENADE_OPTION)) {
            g_Lara.request_gun_type = LGT_GRENADE;
        } else if (KEY_DOWN(DIK_0) && Inv_RequestItem(O_FLARES_OPTION)) {
            g_Lara.request_gun_type = LGT_FLARE;
        }

        if (KEY_DOWN(DIK_8) && Inv_RequestItem(O_SMALL_MEDIPACK_OPTION)) {
            if (m_MediPackCooldown == 0) {
                Lara_UseItem(O_SMALL_MEDIPACK_OPTION);
            }
            m_MediPackCooldown = 15;
        }
        if (KEY_DOWN(DIK_9) && Inv_RequestItem(O_LARGE_MEDIPACK_OPTION)) {
            if (m_MediPackCooldown == 0) {
                Lara_UseItem(O_LARGE_MEDIPACK_OPTION);
            }
            m_MediPackCooldown = 15;
        }

        if (m_MediPackCooldown > 0) {
            m_MediPackCooldown--;
        }
    }

    if (KEY_DOWN(DIK_S)) {
        Screenshot(g_PrimaryBufferSurface);
    }

    const bool is_shift_pressed = KEY_DOWN(DIK_LSHIFT) || KEY_DOWN(DIK_RSHIFT);

    // software renderer keys…
    if (g_SavedAppSettings.render_mode == RM_SOFTWARE) {
        // toggle triple buffering (F7) or perspective correction (Shift+F7)
        if (KEY_DOWN(DIK_F7)) {
            if (!m_IsF7Pressed) {
                m_IsF7Pressed = true;
                APP_SETTINGS new_settings = g_SavedAppSettings;
                if (!is_shift_pressed) {
                    new_settings.perspective_correct =
                        !new_settings.perspective_correct;
                    g_PerspectiveDistance =
                        g_SavedAppSettings.perspective_correct
                        ? SW_DETAIL_HIGH
                        : SW_DETAIL_MEDIUM;
                } else if (g_SavedAppSettings.fullscreen) {
                    new_settings.triple_buffering =
                        !new_settings.triple_buffering;
                }
                GameApplySettings(&new_settings);
            }
        } else {
            m_IsF7Pressed = false;
        }
    }

    // hardware renderer keys…
    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
        // toggle triple buffering (Shift+F7) or zbuffer (F7)
        if (KEY_DOWN(DIK_F7)) {
            if (!m_IsF7Pressed) {
                m_IsF7Pressed = true;
                APP_SETTINGS new_settings = g_SavedAppSettings;
                if (!is_shift_pressed) {
                    new_settings.zbuffer = !new_settings.zbuffer;
                } else if (g_SavedAppSettings.fullscreen) {
                    new_settings.triple_buffering =
                        !new_settings.triple_buffering;
                }
                GameApplySettings(&new_settings);
            }
        } else {
            m_IsF7Pressed = false;
        }

        // toggle perspective correction (Shift+F8) or bilinear filter (F8)
        if (KEY_DOWN(DIK_F8)) {
            if (!m_IsF8Pressed) {
                m_IsF8Pressed = true;
                APP_SETTINGS new_settings = g_SavedAppSettings;
                if (!is_shift_pressed) {
                    new_settings.bilinear_filtering =
                        !new_settings.bilinear_filtering;
                } else {
                    new_settings.perspective_correct =
                        !new_settings.perspective_correct;
                }
                GameApplySettings(&new_settings);
            }
        } else {
            m_IsF8Pressed = false;
        }

        // toggle dither (F11)
        if (KEY_DOWN(DIK_F11)) {
            if (!m_IsF11Pressed) {
                m_IsF11Pressed = true;
                APP_SETTINGS new_settings = g_SavedAppSettings;
                new_settings.dither = !new_settings.dither;
                GameApplySettings(&new_settings);
            }
        } else {
            m_IsF11Pressed = false;
        }
    }

    if (!g_IsVidModeLock && KEY_DOWN(DIK_F12)) {
        APP_SETTINGS new_settings = g_SavedAppSettings;
        // toggle fullscreen (F12)
        if (!is_shift_pressed) {
            new_settings.fullscreen = !new_settings.fullscreen;
            if (g_SavedAppSettings.fullscreen) {
                const int32_t win_width = MAX(g_PhdWinWidth, 320);
                const int32_t win_height = MAX(g_PhdWinHeight, 240);
                new_settings.window_height = win_height;
                new_settings.window_width =
                    CalculateWindowWidth(win_width, win_height);
                new_settings.triple_buffering = 0;
                GameApplySettings(&new_settings);

                g_GameSizer = 1.0;
                g_GameSizerCopy = 1.0;
                setup_screen_size();
            } else {
                const DISPLAY_MODE_LIST *const mode_list =
                    new_settings.render_mode == RM_HARDWARE
                    ? &g_CurrentDisplayAdapter.hw_disp_mode_list
                    : &g_CurrentDisplayAdapter.sw_disp_mode_list;

                if (mode_list->count > 0) {
                    const DISPLAY_MODE target_mode = {
                        .width = g_GameVid_Width,
                        .height = g_GameVid_Height,
                        .bpp = g_GameVid_BPP,
                        .vga = VGA_NO_VGA,
                    };

                    const DISPLAY_MODE_NODE *mode = NULL;
                    for (mode = mode_list->head; mode != NULL;
                         mode = mode->next) {
                        if (!CompareVideoModes(&mode->body, &target_mode)) {
                            break;
                        }
                    }

                    if (mode == NULL) {
                        mode = mode_list->tail;
                    }

                    new_settings.video_mode = mode;
                    GameApplySettings(&new_settings);
                }
            }
            // toggle renderer mode (Shift+F12)
        } else if (!g_Inv_IsActive) {
            new_settings.render_mode = new_settings.render_mode == RM_HARDWARE
                ? RM_SOFTWARE
                : RM_HARDWARE;

            const DISPLAY_MODE_LIST *const mode_list =
                new_settings.render_mode == RM_HARDWARE
                ? &g_CurrentDisplayAdapter.hw_disp_mode_list
                : &g_CurrentDisplayAdapter.sw_disp_mode_list;

            if (mode_list->count > 0) {
                const DISPLAY_MODE target_mode = {
                    .width = g_GameVid_Width,
                    .height = g_GameVid_Height,
                    .bpp = new_settings.render_mode == RM_HARDWARE ? 16 : 8,
                    .vga = VGA_NO_VGA,
                };

                const DISPLAY_MODE_NODE *mode = NULL;
                for (mode = mode_list->head; mode != NULL; mode = mode->next) {
                    if (!CompareVideoModes(&mode->body, &target_mode)) {
                        break;
                    }
                }

                if (mode == NULL) {
                    mode = mode_list->tail;
                }

                new_settings.video_mode = mode;
                new_settings.fullscreen = 1;
                GameApplySettings(&new_settings);
            }
        }
    }

    if (!g_IsVidSizeLock && g_Camera.type != CAM_CINEMATIC
        && !(g_GameFlow.screen_sizing_disabled)) {
        APP_SETTINGS new_settings = g_SavedAppSettings;

        // decrease resolution or BPP (F1)
        if (KEY_DOWN(DIK_F1) && new_settings.fullscreen) {
            const DISPLAY_MODE_NODE *const current_mode =
                new_settings.video_mode;
            const DISPLAY_MODE_NODE *mode = current_mode;
            if (mode != NULL) {
                mode = mode->previous;
            }

            if (new_settings.render_mode == RM_HARDWARE) {
                for (; mode != NULL; mode = mode->previous) {
                    if (is_shift_pressed) {
                        if (mode->body.width == current_mode->body.width
                            && mode->body.height == current_mode->body.height
                            && mode->body.vga == current_mode->body.vga
                            && mode->body.bpp < current_mode->body.bpp) {
                            break;
                        }
                    } else if (
                        mode->body.vga == current_mode->body.vga
                        && mode->body.bpp == current_mode->body.bpp) {
                        break;
                    }
                }
            }

            if (mode != NULL) {
                new_settings.video_mode = mode;
                GameApplySettings(&new_settings);
            }
        }

        // increase resolution (F2)
        if (KEY_DOWN(DIK_F2) && new_settings.fullscreen) {
            const DISPLAY_MODE_NODE *const current_mode =
                new_settings.video_mode;
            const DISPLAY_MODE_NODE *mode = current_mode;
            if (mode != NULL) {
                mode = mode->next;
            }

            if (new_settings.render_mode == RM_HARDWARE) {
                for (; mode != NULL; mode = mode->next) {
                    if (is_shift_pressed) {
                        if (mode->body.width == current_mode->body.width
                            && mode->body.height == current_mode->body.height
                            && mode->body.vga == current_mode->body.vga
                            && mode->body.bpp > current_mode->body.bpp) {
                            break;
                        }
                    } else if (
                        mode->body.vga == current_mode->body.vga
                        && mode->body.bpp == current_mode->body.bpp) {
                        break;
                    }
                }
            }
            if (mode != NULL) {
                new_settings.video_mode = mode;
                GameApplySettings(&new_settings);
            }
        }

        // decrease inner screen size (F3)
        if (KEY_DOWN(DIK_F3)) {
            if (!m_IsF3Pressed) {
                m_IsF3Pressed = true;
                if (g_SavedAppSettings.fullscreen) {
                    DecreaseScreenSize();
                }
            }
        } else {
            m_IsF3Pressed = false;
        }

        // increase inner screen size (F4)
        if (KEY_DOWN(DIK_F4)) {
            if (!m_IsF4Pressed) {
                m_IsF4Pressed = true;
                if (g_SavedAppSettings.fullscreen) {
                    IncreaseScreenSize();
                }
            }
        } else {
            m_IsF4Pressed = false;
        }
    }

    if (!g_GameFlow.load_save_disabled) {
        if (KEY_DOWN(DIK_F5)) {
            input |= IN_SAVE;
        } else if (KEY_DOWN(DIK_F6)) {
            input |= IN_LOAD;
        }
    }

    g_Input = input;
    return g_IsGameToExit;
}

bool Input_IsAnythingPressed(void)
{
    WinInReadKeyboard(g_DIKeys);
    for (int32_t i = 0; i < 256; i++) {
        if (KEY_DOWN(i)) {
            return true;
        }
    }
    return false;
}

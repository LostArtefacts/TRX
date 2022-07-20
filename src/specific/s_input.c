#include "specific/s_input.h"

#include "config.h"
#include "game/input.h"
#include "log.h"
#include "specific/s_shell.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <stdbool.h>
#include <stddef.h>

#define KEY_DOWN(a) (m_KeyboardState[(a)])

static INPUT_SCANCODE m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {
    // clang-format off
    // built-in controls
    {
        SDL_SCANCODE_UP,         // INPUT_ROLE_UP
        SDL_SCANCODE_DOWN,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_LEFT,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_RIGHT,      // INPUT_ROLE_RIGHT
        SDL_SCANCODE_DELETE,     // INPUT_ROLE_STEP_L
        SDL_SCANCODE_PAGEDOWN,   // INPUT_ROLE_STEP_R
        SDL_SCANCODE_RSHIFT,     // INPUT_ROLE_SLOW
        SDL_SCANCODE_RALT,       // INPUT_ROLE_JUMP
        SDL_SCANCODE_RCTRL,      // INPUT_ROLE_ACTION
        SDL_SCANCODE_SPACE,      // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_END,        // INPUT_ROLE_ROLL
        SDL_SCANCODE_ESCAPE,     // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
    },

    // default user controls
    {
        SDL_SCANCODE_KP_8,       // INPUT_ROLE_UP
        SDL_SCANCODE_KP_2,       // INPUT_ROLE_DOWN
        SDL_SCANCODE_KP_4,       // INPUT_ROLE_LEFT
        SDL_SCANCODE_KP_6,       // INPUT_ROLE_RIGHT
        SDL_SCANCODE_KP_7,       // INPUT_ROLE_STEP_L
        SDL_SCANCODE_KP_9,       // INPUT_ROLE_STEP_R
        SDL_SCANCODE_KP_1,       // INPUT_ROLE_SLOW
        SDL_SCANCODE_KP_PLUS,    // INPUT_ROLE_JUMP
        SDL_SCANCODE_KP_ENTER,   // INPUT_ROLE_ACTION
        SDL_SCANCODE_KP_3,       // INPUT_ROLE_DRAW
        SDL_SCANCODE_KP_0,       // INPUT_ROLE_LOOK
        SDL_SCANCODE_KP_5,       // INPUT_ROLE_ROLL
        SDL_SCANCODE_KP_PERIOD,  // INPUT_ROLE_OPTION
        SDL_SCANCODE_O,          // INPUT_ROLE_FLY_CHEAT,
        SDL_SCANCODE_I,          // INPUT_ROLE_ITEM_CHEAT,
        SDL_SCANCODE_L,          // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        SDL_SCANCODE_TAB,        // INPUT_ROLE_TURBO_CHEAT,
        SDL_SCANCODE_P,          // INPUT_ROLE_PAUSE,
        SDL_SCANCODE_W,          // INPUT_ROLE_CAMERA_UP
        SDL_SCANCODE_S,          // INPUT_ROLE_CAMERA_DOWN
        SDL_SCANCODE_A,          // INPUT_ROLE_CAMERA_LEFT
        SDL_SCANCODE_D,          // INPUT_ROLE_CAMERA_RIGHT
        SDL_SCANCODE_SLASH,      // INPUT_ROLE_CAMERA_RESET
    }
    // clang-format on
};

const Uint8 *m_KeyboardState;
static SDL_GameController *m_Controller = NULL;

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode);
static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout);
static bool S_Input_Key(INPUT_ROLE role);
static bool S_Input_JoyBtn(SDL_GameControllerButton button);
static int16_t S_Input_JoyAxis(SDL_GameControllerAxis axis);

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode);

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode)
{
    // clang-format off
    switch (scancode) {
        case SDL_SCANCODE_LCTRL:              return "CTRL";
        case SDL_SCANCODE_RCTRL:              return "CTRL";
        case SDL_SCANCODE_RSHIFT:             return "SHIFT";
        case SDL_SCANCODE_LSHIFT:             return "SHIFT";
        case SDL_SCANCODE_RALT:               return "ALT";
        case SDL_SCANCODE_LALT:               return "ALT";
        case SDL_SCANCODE_LGUI:               return "WIN";
        case SDL_SCANCODE_RGUI:               return "WIN";

        case SDL_SCANCODE_LEFT:               return "LEFT";
        case SDL_SCANCODE_UP:                 return "UP";
        case SDL_SCANCODE_RIGHT:              return "RIGHT";
        case SDL_SCANCODE_DOWN:               return "DOWN";

        case SDL_SCANCODE_RETURN:             return "RET";
        case SDL_SCANCODE_ESCAPE:             return "ESC";
        case SDL_SCANCODE_BACKSPACE:          return "BKSP";
        case SDL_SCANCODE_TAB:                return "TAB";
        case SDL_SCANCODE_SPACE:              return "SPACE";
        case SDL_SCANCODE_CAPSLOCK:           return "CAPS";
        case SDL_SCANCODE_PRINTSCREEN:        return "PSCRN";
        case SDL_SCANCODE_SCROLLLOCK:         return "SCLK";
        case SDL_SCANCODE_PAUSE:              return "PAUSE";
        case SDL_SCANCODE_INSERT:             return "INS";
        case SDL_SCANCODE_HOME:               return "HOME";
        case SDL_SCANCODE_PAGEUP:             return "PGUP";
        case SDL_SCANCODE_DELETE:             return "DEL";
        case SDL_SCANCODE_END:                return "END";
        case SDL_SCANCODE_PAGEDOWN:           return "PGDN";

        case SDL_SCANCODE_A:                  return "A";
        case SDL_SCANCODE_B:                  return "B";
        case SDL_SCANCODE_C:                  return "C";
        case SDL_SCANCODE_D:                  return "D";
        case SDL_SCANCODE_E:                  return "E";
        case SDL_SCANCODE_F:                  return "F";
        case SDL_SCANCODE_G:                  return "G";
        case SDL_SCANCODE_H:                  return "H";
        case SDL_SCANCODE_I:                  return "I";
        case SDL_SCANCODE_J:                  return "J";
        case SDL_SCANCODE_K:                  return "K";
        case SDL_SCANCODE_L:                  return "L";
        case SDL_SCANCODE_M:                  return "M";
        case SDL_SCANCODE_N:                  return "N";
        case SDL_SCANCODE_O:                  return "O";
        case SDL_SCANCODE_P:                  return "P";
        case SDL_SCANCODE_Q:                  return "Q";
        case SDL_SCANCODE_R:                  return "R";
        case SDL_SCANCODE_S:                  return "S";
        case SDL_SCANCODE_T:                  return "T";
        case SDL_SCANCODE_U:                  return "U";
        case SDL_SCANCODE_V:                  return "V";
        case SDL_SCANCODE_W:                  return "W";
        case SDL_SCANCODE_X:                  return "X";
        case SDL_SCANCODE_Y:                  return "Y";
        case SDL_SCANCODE_Z:                  return "Z";

        case SDL_SCANCODE_0:                  return "0";
        case SDL_SCANCODE_1:                  return "1";
        case SDL_SCANCODE_2:                  return "2";
        case SDL_SCANCODE_3:                  return "3";
        case SDL_SCANCODE_4:                  return "4";
        case SDL_SCANCODE_5:                  return "5";
        case SDL_SCANCODE_6:                  return "6";
        case SDL_SCANCODE_7:                  return "7";
        case SDL_SCANCODE_8:                  return "8";
        case SDL_SCANCODE_9:                  return "9";

        case SDL_SCANCODE_MINUS:              return "-";
        case SDL_SCANCODE_EQUALS:             return "=";
        case SDL_SCANCODE_LEFTBRACKET:        return "[";
        case SDL_SCANCODE_RIGHTBRACKET:       return "]";
        case SDL_SCANCODE_BACKSLASH:          return "\\";
        case SDL_SCANCODE_NONUSHASH:          return "#";
        case SDL_SCANCODE_SEMICOLON:          return ";";
        case SDL_SCANCODE_APOSTROPHE:         return "'";
        case SDL_SCANCODE_GRAVE:              return "`";
        case SDL_SCANCODE_COMMA:              return ",";
        case SDL_SCANCODE_PERIOD:             return ".";
        case SDL_SCANCODE_SLASH:              return "/";

        case SDL_SCANCODE_F1:                 return "F1";
        case SDL_SCANCODE_F2:                 return "F2";
        case SDL_SCANCODE_F3:                 return "F3";
        case SDL_SCANCODE_F4:                 return "F4";
        case SDL_SCANCODE_F5:                 return "F5";
        case SDL_SCANCODE_F6:                 return "F6";
        case SDL_SCANCODE_F7:                 return "F7";
        case SDL_SCANCODE_F8:                 return "F8";
        case SDL_SCANCODE_F9:                 return "F9";
        case SDL_SCANCODE_F10:                return "F10";
        case SDL_SCANCODE_F11:                return "F11";
        case SDL_SCANCODE_F12:                return "F12";
        case SDL_SCANCODE_F13:                return "F13";
        case SDL_SCANCODE_F14:                return "F14";
        case SDL_SCANCODE_F15:                return "F15";
        case SDL_SCANCODE_F16:                return "F16";
        case SDL_SCANCODE_F17:                return "F17";
        case SDL_SCANCODE_F18:                return "F18";
        case SDL_SCANCODE_F19:                return "F19";
        case SDL_SCANCODE_F20:                return "F20";
        case SDL_SCANCODE_F21:                return "F21";
        case SDL_SCANCODE_F22:                return "F22";
        case SDL_SCANCODE_F23:                return "F23";
        case SDL_SCANCODE_F24:                return "F24";

        case SDL_SCANCODE_NUMLOCKCLEAR:       return "NMLK";
        case SDL_SCANCODE_KP_0:               return "PAD0";
        case SDL_SCANCODE_KP_1:               return "PAD1";
        case SDL_SCANCODE_KP_2:               return "PAD2";
        case SDL_SCANCODE_KP_3:               return "PAD3";
        case SDL_SCANCODE_KP_4:               return "PAD4";
        case SDL_SCANCODE_KP_5:               return "PAD5";
        case SDL_SCANCODE_KP_6:               return "PAD6";
        case SDL_SCANCODE_KP_7:               return "PAD7";
        case SDL_SCANCODE_KP_8:               return "PAD8";
        case SDL_SCANCODE_KP_9:               return "PAD9";
        case SDL_SCANCODE_KP_PERIOD:          return "PAD.";
        case SDL_SCANCODE_KP_DIVIDE:          return "PAD/";
        case SDL_SCANCODE_KP_MULTIPLY:        return "PAD*";
        case SDL_SCANCODE_KP_MINUS:           return "PAD-";
        case SDL_SCANCODE_KP_PLUS:            return "PAD+";
        case SDL_SCANCODE_KP_EQUALS:          return "PAD=";
        case SDL_SCANCODE_KP_EQUALSAS400:     return "PAD=";
        case SDL_SCANCODE_KP_COMMA:           return "PAD,";
        case SDL_SCANCODE_KP_ENTER:           return "ENTER";

        // extra keys
        case SDL_SCANCODE_APPLICATION:        return "MENU";
        case SDL_SCANCODE_POWER:              return "POWER";
        case SDL_SCANCODE_EXECUTE:            return "EXEC";
        case SDL_SCANCODE_HELP:               return "HELP";
        case SDL_SCANCODE_MENU:               return "MENU";
        case SDL_SCANCODE_SELECT:             return "SEL";
        case SDL_SCANCODE_STOP:               return "STOP";
        case SDL_SCANCODE_AGAIN:              return "AGAIN";
        case SDL_SCANCODE_UNDO:               return "UNDO";
        case SDL_SCANCODE_CUT:                return "CUT";
        case SDL_SCANCODE_COPY:               return "COPY";
        case SDL_SCANCODE_PASTE:              return "PASTE";
        case SDL_SCANCODE_FIND:               return "FIND";
        case SDL_SCANCODE_MUTE:               return "MUTE";
        case SDL_SCANCODE_VOLUMEUP:           return "VOLUP";
        case SDL_SCANCODE_VOLUMEDOWN:         return "VOLDN";
        case SDL_SCANCODE_ALTERASE:           return "ALTER";
        case SDL_SCANCODE_SYSREQ:             return "SYSRQ";
        case SDL_SCANCODE_CANCEL:             return "CNCEL";
        case SDL_SCANCODE_CLEAR:              return "CLEAR";
        case SDL_SCANCODE_PRIOR:              return "PRIOR";
        case SDL_SCANCODE_RETURN2:            return "RETURN";
        case SDL_SCANCODE_SEPARATOR:          return "SEP";
        case SDL_SCANCODE_OUT:                return "OUT";
        case SDL_SCANCODE_OPER:               return "OPER";
        case SDL_SCANCODE_CLEARAGAIN:         return "CLEAR";
        case SDL_SCANCODE_CRSEL:              return "CRSEL";
        case SDL_SCANCODE_EXSEL:              return "EXSEL";
        case SDL_SCANCODE_KP_00:              return "PAD00";
        case SDL_SCANCODE_KP_000:             return "PAD000";
        case SDL_SCANCODE_THOUSANDSSEPARATOR: return "TSEP";
        case SDL_SCANCODE_DECIMALSEPARATOR:   return "DSEP";
        case SDL_SCANCODE_CURRENCYUNIT:       return "CURU";
        case SDL_SCANCODE_CURRENCYSUBUNIT:    return "CURSU";
        case SDL_SCANCODE_KP_LEFTPAREN:       return "PAD(";
        case SDL_SCANCODE_KP_RIGHTPAREN:      return "PAD)";
        case SDL_SCANCODE_KP_LEFTBRACE:       return "PAD{";
        case SDL_SCANCODE_KP_RIGHTBRACE:      return "PAD}";
        case SDL_SCANCODE_KP_TAB:             return "PADT";
        case SDL_SCANCODE_KP_BACKSPACE:       return "PADBK";
        case SDL_SCANCODE_KP_A:               return "PADA";
        case SDL_SCANCODE_KP_B:               return "PADB";
        case SDL_SCANCODE_KP_C:               return "PADC";
        case SDL_SCANCODE_KP_D:               return "PADD";
        case SDL_SCANCODE_KP_E:               return "PADE";
        case SDL_SCANCODE_KP_F:               return "PADF";
        case SDL_SCANCODE_KP_XOR:             return "PADXR";
        case SDL_SCANCODE_KP_POWER:           return "PAD^";
        case SDL_SCANCODE_KP_PERCENT:         return "PAD%";
        case SDL_SCANCODE_KP_LESS:            return "PAD<";
        case SDL_SCANCODE_KP_GREATER:         return "PAD>";
        case SDL_SCANCODE_KP_AMPERSAND:       return "PAD&";
        case SDL_SCANCODE_KP_DBLAMPERSAND:    return "PAD&&";
        case SDL_SCANCODE_KP_VERTICALBAR:     return "PAD|";
        case SDL_SCANCODE_KP_DBLVERTICALBAR:  return "PAD||";
        case SDL_SCANCODE_KP_COLON:           return "PAD:";
        case SDL_SCANCODE_KP_HASH:            return "PAD#";
        case SDL_SCANCODE_KP_SPACE:           return "PADSP";
        case SDL_SCANCODE_KP_AT:              return "PAD@";
        case SDL_SCANCODE_KP_EXCLAM:          return "PAD!";
        case SDL_SCANCODE_KP_MEMSTORE:        return "PADMS";
        case SDL_SCANCODE_KP_MEMRECALL:       return "PADMR";
        case SDL_SCANCODE_KP_MEMCLEAR:        return "PADMC";
        case SDL_SCANCODE_KP_MEMADD:          return "PADMA";
        case SDL_SCANCODE_KP_MEMSUBTRACT:     return "PADM-";
        case SDL_SCANCODE_KP_MEMMULTIPLY:     return "PADM*";
        case SDL_SCANCODE_KP_MEMDIVIDE:       return "PADM/";
        case SDL_SCANCODE_KP_PLUSMINUS:       return "PAD+-";
        case SDL_SCANCODE_KP_CLEAR:           return "PADCL";
        case SDL_SCANCODE_KP_CLEARENTRY:      return "PADCL";
        case SDL_SCANCODE_KP_BINARY:          return "PAD02";
        case SDL_SCANCODE_KP_OCTAL:           return "PAD08";
        case SDL_SCANCODE_KP_DECIMAL:         return "PAD10";
        case SDL_SCANCODE_KP_HEXADECIMAL:     return "PAD16";
        case SDL_SCANCODE_MODE:               return "MODE";
        case SDL_SCANCODE_AUDIONEXT:          return "NEXT";
        case SDL_SCANCODE_AUDIOPREV:          return "PREV";
        case SDL_SCANCODE_AUDIOSTOP:          return "STOP";
        case SDL_SCANCODE_AUDIOPLAY:          return "PLAY";
        case SDL_SCANCODE_AUDIOMUTE:          return "MUTE";
        case SDL_SCANCODE_MEDIASELECT:        return "MEDIA";
        case SDL_SCANCODE_WWW:                return "WWW";
        case SDL_SCANCODE_MAIL:               return "MAIL";
        case SDL_SCANCODE_CALCULATOR:         return "CALC";
        case SDL_SCANCODE_COMPUTER:           return "COMP";
        case SDL_SCANCODE_AC_SEARCH:          return "SRCH";
        case SDL_SCANCODE_AC_HOME:            return "HOME";
        case SDL_SCANCODE_AC_BACK:            return "BACK";
        case SDL_SCANCODE_AC_FORWARD:         return "FRWD";
        case SDL_SCANCODE_AC_STOP:            return "STOP";
        case SDL_SCANCODE_AC_REFRESH:         return "RFRSH";
        case SDL_SCANCODE_AC_BOOKMARKS:       return "BKMK";
        case SDL_SCANCODE_BRIGHTNESSDOWN:     return "BNDN";
        case SDL_SCANCODE_BRIGHTNESSUP:       return "BNUP";
        case SDL_SCANCODE_DISPLAYSWITCH:      return "DPSW";
        case SDL_SCANCODE_KBDILLUMTOGGLE:     return "KBDIT";
        case SDL_SCANCODE_KBDILLUMDOWN:       return "KBDID";
        case SDL_SCANCODE_KBDILLUMUP:         return "KBDIU";
        case SDL_SCANCODE_EJECT:              return "EJECT";
        case SDL_SCANCODE_SLEEP:              return "SLEEP";
        case SDL_SCANCODE_APP1:               return "APP1";
        case SDL_SCANCODE_APP2:               return "APP2";
        case SDL_SCANCODE_AUDIOREWIND:        return "RWND";
        case SDL_SCANCODE_AUDIOFASTFORWARD:   return "FF";
    }
    // clang-format on
    return "????";
}

static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout)
{
    INPUT_SCANCODE scancode = m_Layout[layout][role];
    if (KEY_DOWN(scancode)) {
        return true;
    }
    if (scancode == SDL_SCANCODE_LCTRL) {
        return KEY_DOWN(SDL_SCANCODE_RCTRL);
    }
    if (scancode == SDL_SCANCODE_RCTRL) {
        return KEY_DOWN(SDL_SCANCODE_LCTRL);
    }
    if (scancode == SDL_SCANCODE_LSHIFT) {
        return KEY_DOWN(SDL_SCANCODE_RSHIFT);
    }
    if (scancode == SDL_SCANCODE_RSHIFT) {
        return KEY_DOWN(SDL_SCANCODE_LSHIFT);
    }
    if (scancode == SDL_SCANCODE_LALT) {
        return KEY_DOWN(SDL_SCANCODE_RALT);
    }
    if (scancode == SDL_SCANCODE_RALT) {
        return KEY_DOWN(SDL_SCANCODE_LALT);
    }
    return false;
}

static bool S_Input_Key(INPUT_ROLE role)
{
    return S_Input_KbdKey(role, INPUT_LAYOUT_USER)
        || (!Input_IsKeyConflictedWithDefault(role)
            && S_Input_KbdKey(role, INPUT_LAYOUT_DEFAULT));
}

static bool S_Input_JoyBtn(SDL_GameControllerButton button)
{
    return SDL_GameControllerGetButton(m_Controller, button);
}

static int16_t S_Input_JoyAxis(SDL_GameControllerAxis axis)
{
    Sint16 value = SDL_GameControllerGetAxis(m_Controller, axis);
    if (value < -SDL_JOYSTICK_AXIS_MAX / 2) {
        return -1;
    }
    if (value > SDL_JOYSTICK_AXIS_MAX / 2) {
        return 1;
    }
    return 0;
}

void S_Input_Init(void)
{
    m_KeyboardState = SDL_GetKeyboardState(NULL);

    int32_t result = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
    } else {
        int controllers = SDL_NumJoysticks();
        LOG_INFO("%d controllers", controllers);
        for (int i = 0; i < controllers; i++) {
            const char *name = SDL_GameControllerNameForIndex(i);
            bool is_game_controller = SDL_IsGameController(i);
            LOG_DEBUG("controller %d: %s (%d)", i, name, is_game_controller);
            if (!m_Controller && is_game_controller) {
                m_Controller = SDL_GameControllerOpen(i);
                if (!m_Controller) {
                    LOG_ERROR("Could not open controller: %s", SDL_GetError());
                }
            }
        }
    }
}

void S_Input_Shutdown(void)
{
    if (m_Controller) {
        SDL_GameControllerClose(m_Controller);
    }
}

INPUT_STATE S_Input_GetCurrentState(void)
{
    INPUT_STATE linput = { 0 };

    // clang-format off
    linput.forward                   = S_Input_Key(INPUT_ROLE_UP);
    linput.back                      = S_Input_Key(INPUT_ROLE_DOWN);
    linput.left                      = S_Input_Key(INPUT_ROLE_LEFT);
    linput.right                     = S_Input_Key(INPUT_ROLE_RIGHT);
    linput.step_left                 = S_Input_Key(INPUT_ROLE_STEP_L);
    linput.step_right                = S_Input_Key(INPUT_ROLE_STEP_R);
    linput.slow                      = S_Input_Key(INPUT_ROLE_SLOW);
    linput.jump                      = S_Input_Key(INPUT_ROLE_JUMP);
    linput.action                    = S_Input_Key(INPUT_ROLE_ACTION);
    linput.draw                      = S_Input_Key(INPUT_ROLE_DRAW);
    linput.look                      = S_Input_Key(INPUT_ROLE_LOOK);
    linput.roll                      = S_Input_Key(INPUT_ROLE_ROLL);
    linput.option                    = S_Input_Key(INPUT_ROLE_OPTION);
    linput.pause                     = S_Input_Key(INPUT_ROLE_PAUSE);
    linput.camera_up                 = S_Input_Key(INPUT_ROLE_CAMERA_UP);
    linput.camera_down               = S_Input_Key(INPUT_ROLE_CAMERA_DOWN);
    linput.camera_left               = S_Input_Key(INPUT_ROLE_CAMERA_LEFT);
    linput.camera_right              = S_Input_Key(INPUT_ROLE_CAMERA_RIGHT);
    linput.camera_reset              = S_Input_Key(INPUT_ROLE_CAMERA_RESET);

    linput.item_cheat                = S_Input_Key(INPUT_ROLE_ITEM_CHEAT);
    linput.fly_cheat                 = S_Input_Key(INPUT_ROLE_FLY_CHEAT);
    linput.level_skip_cheat          = S_Input_Key(INPUT_ROLE_LEVEL_SKIP_CHEAT);
    linput.turbo_cheat               = S_Input_Key(INPUT_ROLE_TURBO_CHEAT);
    linput.health_cheat              = KEY_DOWN(SDL_SCANCODE_F11);

    linput.equip_pistols             = KEY_DOWN(SDL_SCANCODE_1);
    linput.equip_shotgun             = KEY_DOWN(SDL_SCANCODE_2);
    linput.equip_magnums             = KEY_DOWN(SDL_SCANCODE_3);
    linput.equip_uzis                = KEY_DOWN(SDL_SCANCODE_4);
    linput.use_small_medi            = KEY_DOWN(SDL_SCANCODE_8);
    linput.use_big_medi              = KEY_DOWN(SDL_SCANCODE_9);

    linput.select                    = KEY_DOWN(SDL_SCANCODE_RETURN);
    linput.deselect                  = S_Input_Key(INPUT_ROLE_OPTION);

    linput.save                      = KEY_DOWN(SDL_SCANCODE_F5);
    linput.load                      = KEY_DOWN(SDL_SCANCODE_F6);

    linput.toggle_fps_counter        = KEY_DOWN(SDL_SCANCODE_F2);
    linput.toggle_bilinear_filter    = KEY_DOWN(SDL_SCANCODE_F3);
    linput.toggle_perspective_filter = KEY_DOWN(SDL_SCANCODE_F4);
    // clang-format on

    if (g_Config.enable_buffering
        && (KEY_DOWN(SDL_SCANCODE_F2) || KEY_DOWN(SDL_SCANCODE_F3)
            || KEY_DOWN(SDL_SCANCODE_F4))) {
        while (KEY_DOWN(SDL_SCANCODE_F2) || KEY_DOWN(SDL_SCANCODE_F3)
               || KEY_DOWN(SDL_SCANCODE_F4)) {
            S_Shell_SpinMessageLoop();
        }
    }

    if (m_Controller) {
        // clang-format off
        linput.forward      |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_LEFTY) < 0;
        linput.back         |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_LEFTY) > 0;
        linput.left         |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_LEFTX) < 0;
        linput.right        |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_LEFTX) > 0;
        linput.step_left    |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 0;
        linput.step_right   |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0;
        linput.camera_left  |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_RIGHTX) < 0;
        linput.camera_right |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_RIGHTX) > 0;
        linput.camera_up    |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_RIGHTY) < 0;
        linput.camera_down  |= S_Input_JoyAxis(SDL_CONTROLLER_AXIS_RIGHTY) > 0;
        linput.forward      |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_DPAD_UP);
        linput.right        |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        linput.left         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        linput.back         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        linput.action       |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_A);
        linput.select       |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_A);
        linput.roll         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_B);
        linput.deselect     |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_B);
        linput.jump         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_X);
        linput.draw         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_Y);
        linput.look         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        linput.slow         |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        linput.deselect     |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_BACK);
        linput.option       |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_BACK);
        linput.deselect     |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_TOUCHPAD);
        linput.option       |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_TOUCHPAD);
        linput.pause        |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_START);
        linput.deselect     |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_START);
        linput.camera_reset |= S_Input_JoyBtn(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        // clang-format on
    }

    return linput;
}

INPUT_SCANCODE S_Input_GetAssignedScancode(int16_t layout_num, INPUT_ROLE role)
{
    return m_Layout[layout_num][role];
}

void S_Input_AssignScancode(
    int16_t layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode)
{
    m_Layout[layout_num][role] = scancode;
}

bool S_Input_ReadAndAssignKey(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    for (INPUT_SCANCODE scancode = 0; scancode < SDL_NUM_SCANCODES;
         scancode++) {
        if (KEY_DOWN(scancode)) {
            m_Layout[layout_num][role] = scancode;
            return true;
        }
    }
    return false;
}

const char *S_Input_GetKeyName(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetScancodeName(m_Layout[layout_num][role]);
}

#include "game/input/keyboard.h"

#include "game/input/internal.h"

#include <SDL2/SDL_keyboard.h>

#define KEY_DOWN(a) (m_KeyboardState[(a)])

typedef struct {
    INPUT_ROLE role;
    SDL_Scancode scancode;
} BUILTIN_KEYBOARD_LAYOUT;

const Uint8 *m_KeyboardState;
static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = { 0 };

static BUILTIN_KEYBOARD_LAYOUT m_BuiltinLayout[] = {
    // clang-format off
    { INPUT_ROLE_UP,                SDL_SCANCODE_UP },
    { INPUT_ROLE_DOWN,              SDL_SCANCODE_DOWN },
    { INPUT_ROLE_LEFT,              SDL_SCANCODE_LEFT },
    { INPUT_ROLE_RIGHT,             SDL_SCANCODE_RIGHT },
    { INPUT_ROLE_STEP_L,            SDL_SCANCODE_DELETE },
    { INPUT_ROLE_STEP_R,            SDL_SCANCODE_PAGEDOWN },
    { INPUT_ROLE_SLOW,              SDL_SCANCODE_RSHIFT },
    { INPUT_ROLE_JUMP,              SDL_SCANCODE_RALT },
    { INPUT_ROLE_ACTION,            SDL_SCANCODE_RCTRL },
    { INPUT_ROLE_DRAW,              SDL_SCANCODE_SPACE },
    { INPUT_ROLE_LOOK,              SDL_SCANCODE_KP_0 },
    { INPUT_ROLE_ROLL,              SDL_SCANCODE_END },
    { INPUT_ROLE_OPTION,            SDL_SCANCODE_ESCAPE },
    { INPUT_ROLE_FLY_CHEAT,         SDL_SCANCODE_O },
    { INPUT_ROLE_ITEM_CHEAT,        SDL_SCANCODE_I },
    { INPUT_ROLE_LEVEL_SKIP_CHEAT,  SDL_SCANCODE_L },
    { INPUT_ROLE_TURBO_CHEAT,       SDL_SCANCODE_TAB },
    { INPUT_ROLE_PAUSE,             SDL_SCANCODE_P },
    { INPUT_ROLE_CAMERA_FORWARD,    SDL_SCANCODE_W },
    { INPUT_ROLE_CAMERA_BACK,       SDL_SCANCODE_S },
    { INPUT_ROLE_CAMERA_LEFT,       SDL_SCANCODE_A },
    { INPUT_ROLE_CAMERA_RIGHT,      SDL_SCANCODE_D },
    { INPUT_ROLE_EQUIP_PISTOLS,     SDL_SCANCODE_1 },
    { INPUT_ROLE_EQUIP_SHOTGUN,     SDL_SCANCODE_2 },
    { INPUT_ROLE_EQUIP_MAGNUMS,     SDL_SCANCODE_3 },
    { INPUT_ROLE_EQUIP_UZIS,        SDL_SCANCODE_4 },
    { INPUT_ROLE_USE_SMALL_MEDI,    SDL_SCANCODE_8 },
    { INPUT_ROLE_USE_BIG_MEDI,      SDL_SCANCODE_9 },
    { INPUT_ROLE_SAVE,              SDL_SCANCODE_F5 },
    { INPUT_ROLE_LOAD,              SDL_SCANCODE_F6 },
    { INPUT_ROLE_FPS,               SDL_SCANCODE_F2 },
    { INPUT_ROLE_BILINEAR,          SDL_SCANCODE_F3 },
    { INPUT_ROLE_ENTER_CONSOLE,     SDL_SCANCODE_SLASH },
    { INPUT_ROLE_CHANGE_TARGET,     SDL_SCANCODE_Z },
    { INPUT_ROLE_TOGGLE_UI,         SDL_SCANCODE_H },
    { INPUT_ROLE_CAMERA_UP,         SDL_SCANCODE_Q },
    { INPUT_ROLE_CAMERA_DOWN,       SDL_SCANCODE_E },
    { INPUT_ROLE_TOGGLE_PHOTO_MODE, SDL_SCANCODE_F1 },
    { INPUT_ROLE_UNBIND_KEY,        SDL_SCANCODE_BACKSPACE },
    { INPUT_ROLE_RESET_BINDINGS,    SDL_SCANCODE_R },
    { -1, SDL_SCANCODE_UNKNOWN },
    // clang-format on
};

static SDL_Scancode m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF];

static const char *M_GetScancodeName(SDL_Scancode scancode);

static bool M_Key(INPUT_LAYOUT layout, INPUT_ROLE role);
static SDL_Scancode M_GetAssignedScancode(INPUT_LAYOUT layout, INPUT_ROLE role);
static void M_AssignScancode(
    INPUT_LAYOUT layout, INPUT_ROLE role, SDL_Scancode scancode);
static bool M_CheckConflict(
    INPUT_LAYOUT layout, INPUT_ROLE role1, INPUT_ROLE role2);
static void M_AssignConflict(
    INPUT_LAYOUT layout, INPUT_ROLE role, bool conflict);
static void M_CheckConflicts(INPUT_LAYOUT layout);

static void M_Init(void);
static bool M_Update(INPUT_STATE *result, INPUT_LAYOUT layout);
static bool M_IsPressed(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_IsRoleConflicted(INPUT_LAYOUT layout, INPUT_ROLE role);
static const char *M_GetName(INPUT_LAYOUT layout, INPUT_ROLE role);
static void M_UnassignRole(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_AssignFromJSONObject(INPUT_LAYOUT layout, JSON_OBJECT *bind_obj);
static bool M_AssignToJSONObject(
    INPUT_LAYOUT layout, JSON_OBJECT *bind_obj, INPUT_ROLE role);
static void M_ResetLayout(INPUT_LAYOUT layout);
static bool M_ReadAndAssign(INPUT_LAYOUT layout, INPUT_ROLE role);

static const char *M_GetScancodeName(SDL_Scancode scancode)
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
        case SDL_SCANCODE_UNKNOWN:            return "";

        default:                              return "????";
    }
    // clang-format on
}

static bool M_Key(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    SDL_Scancode scancode = m_Layout[layout][role];
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

static SDL_Scancode M_GetAssignedScancode(INPUT_LAYOUT layout, INPUT_ROLE role)
{
    return m_Layout[layout][role];
}

static void M_AssignScancode(
    const INPUT_LAYOUT layout, const INPUT_ROLE role,
    const SDL_Scancode scancode)
{
    m_Layout[layout][role] = scancode;
    M_CheckConflicts(layout);
}

static bool M_CheckConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role1, const INPUT_ROLE role2)
{
    const SDL_Scancode scancode1 = M_GetAssignedScancode(layout, role1);
    const SDL_Scancode scancode2 = M_GetAssignedScancode(layout, role2);
    return scancode1 == scancode2;
}

static void M_AssignConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, bool conflict)
{
    m_Conflicts[layout][role] = conflict;
}

static void M_CheckConflicts(const INPUT_LAYOUT layout)
{
    Input_ConflictHelper(layout, M_CheckConflict, M_AssignConflict);
}

static void M_Init(void)
{
    m_KeyboardState = SDL_GetKeyboardState(NULL);

    for (int32_t layout = INPUT_LAYOUT_DEFAULT; layout < INPUT_LAYOUT_NUMBER_OF;
         layout++) {
        for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
            const BUILTIN_KEYBOARD_LAYOUT *const builtin = &m_BuiltinLayout[i];
            m_Layout[layout][builtin->role] = builtin->scancode;
        }
        M_CheckConflicts(layout);
    }
}

static bool M_Update(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    // clang-format off
    result->forward                   = M_Key(layout, INPUT_ROLE_UP);
    result->back                      = M_Key(layout, INPUT_ROLE_DOWN);
    result->left                      = M_Key(layout, INPUT_ROLE_LEFT);
    result->right                     = M_Key(layout, INPUT_ROLE_RIGHT);
    result->step_left                 = M_Key(layout, INPUT_ROLE_STEP_L);
    result->step_right                = M_Key(layout, INPUT_ROLE_STEP_R);
    result->slow                      = M_Key(layout, INPUT_ROLE_SLOW);
    result->jump                      = M_Key(layout, INPUT_ROLE_JUMP);
    result->action                    = M_Key(layout, INPUT_ROLE_ACTION);
    result->draw                      = M_Key(layout, INPUT_ROLE_DRAW);
    result->look                      = M_Key(layout, INPUT_ROLE_LOOK);
    result->roll                      = M_Key(layout, INPUT_ROLE_ROLL);
    result->option                    = M_Key(layout, INPUT_ROLE_OPTION);
    result->pause                     = M_Key(layout, INPUT_ROLE_PAUSE);
    result->toggle_photo_mode         = M_Key(layout, INPUT_ROLE_TOGGLE_PHOTO_MODE);
    result->camera_up                 = M_Key(layout, INPUT_ROLE_CAMERA_UP);
    result->camera_down               = M_Key(layout, INPUT_ROLE_CAMERA_DOWN);
    result->camera_forward            = M_Key(layout, INPUT_ROLE_CAMERA_FORWARD);
    result->camera_back               = M_Key(layout, INPUT_ROLE_CAMERA_BACK);
    result->camera_left               = M_Key(layout, INPUT_ROLE_CAMERA_LEFT);
    result->camera_right              = M_Key(layout, INPUT_ROLE_CAMERA_RIGHT);
    result->enter_console             = M_Key(layout, INPUT_ROLE_ENTER_CONSOLE);
    result->change_target             = M_Key(layout, INPUT_ROLE_CHANGE_TARGET);

    result->item_cheat                = M_Key(layout, INPUT_ROLE_ITEM_CHEAT);
    result->fly_cheat                 = M_Key(layout, INPUT_ROLE_FLY_CHEAT);
    result->level_skip_cheat          = M_Key(layout, INPUT_ROLE_LEVEL_SKIP_CHEAT);
    result->turbo_cheat               = M_Key(layout, INPUT_ROLE_TURBO_CHEAT);
    result->health_cheat              = KEY_DOWN(SDL_SCANCODE_F11);

    result->equip_pistols             = M_Key(layout, INPUT_ROLE_EQUIP_PISTOLS);
    result->equip_shotgun             = M_Key(layout, INPUT_ROLE_EQUIP_SHOTGUN);
    result->equip_magnums             = M_Key(layout, INPUT_ROLE_EQUIP_MAGNUMS);
    result->equip_uzis                = M_Key(layout, INPUT_ROLE_EQUIP_UZIS);
    result->use_small_medi            = M_Key(layout, INPUT_ROLE_USE_SMALL_MEDI);
    result->use_big_medi              = M_Key(layout, INPUT_ROLE_USE_BIG_MEDI);

    result->menu_up                   = KEY_DOWN(SDL_SCANCODE_UP);
    result->menu_down                 = KEY_DOWN(SDL_SCANCODE_DOWN);
    result->menu_left                 = KEY_DOWN(SDL_SCANCODE_LEFT);
    result->menu_right                = KEY_DOWN(SDL_SCANCODE_RIGHT);
    result->menu_confirm              = KEY_DOWN(SDL_SCANCODE_RETURN) && !KEY_DOWN(SDL_SCANCODE_LALT);
    result->menu_confirm             |= result->action; // we only do this for keyboard input
    result->menu_back                 = KEY_DOWN(SDL_SCANCODE_ESCAPE);

    result->save                      = M_Key(layout, INPUT_ROLE_SAVE);
    result->load                      = M_Key(layout, INPUT_ROLE_LOAD);

    result->toggle_fps_counter        = M_Key(layout, INPUT_ROLE_FPS);
    result->toggle_bilinear_filter    = M_Key(layout, INPUT_ROLE_BILINEAR);
    result->toggle_perspective_filter = KEY_DOWN(SDL_SCANCODE_F4);
    result->toggle_ui                 = M_Key(layout, INPUT_ROLE_TOGGLE_UI);
    // clang-format on

    return true;
}

static bool M_IsPressed(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return M_Key(layout, role);
}

static bool M_IsRoleConflicted(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Conflicts[layout][role];
}

static const char *M_GetName(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return M_GetScancodeName(m_Layout[layout][role]);
}

static void M_UnassignRole(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    M_AssignScancode(layout, role, SDL_SCANCODE_UNKNOWN);
}

static bool M_AssignFromJSONObject(
    const INPUT_LAYOUT layout, JSON_OBJECT *const bind_obj)
{
    const INPUT_ROLE role = JSON_ObjectGetInt(bind_obj, "role", -1);
    if (role == (INPUT_ROLE)-1) {
        return false;
    }

    const SDL_Scancode default_scancode = M_GetAssignedScancode(layout, role);
    const SDL_Scancode user_scancode =
        JSON_ObjectGetInt(bind_obj, "scancode", default_scancode);
    M_AssignScancode(layout, role, user_scancode);
    return true;
}

static bool M_AssignToJSONObject(
    const INPUT_LAYOUT layout, JSON_OBJECT *const bind_obj,
    const INPUT_ROLE role)
{
    const SDL_Scancode default_scancode =
        M_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role);
    const SDL_Scancode user_scancode = M_GetAssignedScancode(layout, role);

    if (user_scancode == default_scancode) {
        return false;
    }

    JSON_ObjectAppendInt(bind_obj, "role", role);
    JSON_ObjectAppendInt(bind_obj, "scancode", user_scancode);
    return true;
}

static void M_ResetLayout(const INPUT_LAYOUT layout)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const SDL_Scancode scancode =
            M_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role);
        M_AssignScancode(layout, role, scancode);
    }
}

static bool M_ReadAndAssign(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (SDL_Scancode scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++) {
        if (KEY_DOWN(scancode)) {
            M_AssignScancode(layout, role, scancode);
            return true;
        }
    }
    return false;
}

INPUT_BACKEND_IMPL g_InputKeyboard = {
    .init = M_Init,
    .shutdown = NULL,
    .update = M_Update,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = M_IsRoleConflicted,
    .get_name = M_GetName,
    .unassign_role = M_UnassignRole,
    .assign_from_json_object = M_AssignFromJSONObject,
    .assign_to_json_object = M_AssignToJSONObject,
    .reset_layout = M_ResetLayout,
    .read_and_assign = M_ReadAndAssign,
};

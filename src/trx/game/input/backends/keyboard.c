#include <trx/game/input/backends/keyboard.h>

#include <trx/core/utils.h>
#include <trx/game/input/backends/internal.h>
#include <trx/game/input/combo.h>
#include <trx/game/input/names.h>
#include <trx/version.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <string.h>

// Key state table updated via SDL events.
#define KEY_DOWN(a) (m_KeyboardState[(a)])

typedef struct {
    int32_t key_count;
    SDL_Scancode keys[INPUT_COMBO_MAX_KEYS];
} KEYBOARD_BINDING;

typedef struct {
    INPUT_ROLE role;
    KEYBOARD_BINDING bind;
} BUILTIN_KEYBOARD_LAYOUT;

typedef struct {
    KEYBOARD_BINDING slots[INPUT_BINDING_SLOTS];
} KEYBOARD_ROLE_BINDING;

static bool m_KeyboardState[SDL_NUM_SCANCODES] = {};
static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {};

static const BUILTIN_KEYBOARD_LAYOUT m_BuiltinLayoutBase[] = {
// clang-format off
#define INPUT_KEYBOARD_ASSIGN(role, key) \
    { role, { (key) != SDL_SCANCODE_UNKNOWN ? 1 : 0, { key } } },
#define INPUT_KEYBOARD_ASSIGN_COMBO(role, key1, key2) \
    { role, { 2, { key1, key2 } } },
#include <trx/game/input/backends/keyboard.def>
    { -1, {} },
    // clang-format on
};

static BUILTIN_KEYBOARD_LAYOUT m_BuiltinLayout[ARRAY_SIZE(m_BuiltinLayoutBase)];

static KEYBOARD_ROLE_BINDING m_Layout[INPUT_LAYOUT_NUMBER_OF]
                                     [INPUT_ROLE_NUMBER_OF];

// Per-scancode tracking for combo prefix deferral.
static bool m_PrefixWasHeld[SDL_NUM_SCANCODES];
static bool m_PrefixComboFired[SDL_NUM_SCANCODES];

// Per-scancode press-tick table. Records the tick each key most recently
// transitioned from released to held, so combos that start on keys the
// user was already holding can be rejected.
static uint32_t m_KeyDownTick[SDL_NUM_SCANCODES];
static uint32_t m_Tick;

// Per-role deferral tracking for combo disambiguation.
static bool m_RoleWasActive[INPUT_ROLE_NUMBER_OF];

static bool m_RoleLongerFired[INPUT_ROLE_NUMBER_OF];

// Combo capture state for listen mode.
static KEYBOARD_BINDING m_CaptureBuffer = { .key_count = 0 };

static bool m_CaptureActive = false;

// Keycaps for the printable characters TRX has glyphs for, keyed by the
// character rather than by scancode so that a key can be labelled with the
// character the active keyboard layout makes it produce.
static const struct {
    char chr;
    const char *glyph;
} m_CharGlyphs[] = {
    // clang-format off
    { 'a',  "\\{keyboard a}" },
    { 'b',  "\\{keyboard b}" },
    { 'c',  "\\{keyboard c}" },
    { 'd',  "\\{keyboard d}" },
    { 'e',  "\\{keyboard e}" },
    { 'f',  "\\{keyboard f}" },
    { 'g',  "\\{keyboard g}" },
    { 'h',  "\\{keyboard h}" },
    { 'i',  "\\{keyboard i}" },
    { 'j',  "\\{keyboard j}" },
    { 'k',  "\\{keyboard k}" },
    { 'l',  "\\{keyboard l}" },
    { 'm',  "\\{keyboard m}" },
    { 'n',  "\\{keyboard n}" },
    { 'o',  "\\{keyboard o}" },
    { 'p',  "\\{keyboard p}" },
    { 'q',  "\\{keyboard q}" },
    { 'r',  "\\{keyboard r}" },
    { 's',  "\\{keyboard s}" },
    { 't',  "\\{keyboard t}" },
    { 'u',  "\\{keyboard u}" },
    { 'v',  "\\{keyboard v}" },
    { 'w',  "\\{keyboard w}" },
    { 'x',  "\\{keyboard x}" },
    { 'y',  "\\{keyboard y}" },
    { 'z',  "\\{keyboard z}" },

    { '0',  "\\{keyboard 0}" },
    { '1',  "\\{keyboard 1}" },
    { '2',  "\\{keyboard 2}" },
    { '3',  "\\{keyboard 3}" },
    { '4',  "\\{keyboard 4}" },
    { '5',  "\\{keyboard 5}" },
    { '6',  "\\{keyboard 6}" },
    { '7',  "\\{keyboard 7}" },
    { '8',  "\\{keyboard 8}" },
    { '9',  "\\{keyboard 9}" },

    { '-',  "\\{keyboard minus}" },
    { '=',  "\\{keyboard equals}" },
    { '[',  "\\{keyboard left_square_bracket}" },
    { ']',  "\\{keyboard right_square_bracket}" },
    { '\\', "\\{keyboard backslash}" },
    { '#',  "\\{keyboard hash}" },
    { ';',  "\\{keyboard semicolon}" },
    { '\'', "\\{keyboard apostrophe}" },
    { '`',  "\\{keyboard backtick}" },
    { ',',  "\\{keyboard comma}" },
    { '.',  "\\{keyboard period}" },
    { '/',  "\\{keyboard slash}" },
    // clang-format on
};

// Update internal controller button/axis state from SDL events.
// @param event     Event to process.
static void M_ProcessEvent(const SDL_Event *const event)
{
    switch (event->type) {
    case SDL_KEYDOWN:
        if (!event->key.repeat) {
            m_KeyboardState[event->key.keysym.scancode] = true;
        }
        break;
    case SDL_KEYUP:
        m_KeyboardState[event->key.keysym.scancode] = false;
        break;
    default:
        break;
    }
}

// Keycap for a printable character, or nullptr if TRX has no glyph for it.
// Accented, Cyrillic and Greek characters all land in the latter case.
static const char *M_GetCharName(const char chr)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_CharGlyphs); i++) {
        if (m_CharGlyphs[i].chr == chr) {
            return m_CharGlyphs[i].glyph;
        }
    }
    return nullptr;
}

// Whether the character printed on a key varies with the keyboard layout.
// Letters permute wholesale between layouts (QWERTY, QWERTZ, AZERTY, Dvorak)
// and the punctuation block moves around with them. The digit row is
// deliberately left out: it reads 1 to 0 on every Latin layout.
static bool M_IsLayoutDependent(const SDL_Scancode scancode)
{
    return (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        || (scancode >= SDL_SCANCODE_MINUS && scancode <= SDL_SCANCODE_SLASH)
        || scancode == SDL_SCANCODE_NONUSBACKSLASH;
}

static const char *M_GetScancodeName(SDL_Scancode scancode)
{
    // Scancodes name physical key positions after a US layout, so on QWERTZ
    // the key labelled Z reports SDL_SCANCODE_Y. Ask SDL which character the
    // key produces under the active layout and label it with that, falling
    // back to the physical name when that character has no keycap glyph.
    if (M_IsLayoutDependent(scancode)) {
        const SDL_Keycode key = SDL_GetKeyFromScancode(scancode);
        if (key > 0 && key < 0x80) {
            const char *const name = M_GetCharName((char)key);
            if (name != nullptr) {
                return name;
            }
        }
    }

    // clang-format off
    switch (scancode) {
        case SDL_SCANCODE_LCTRL:              return "\\{keyboard l_ctrl}";
        case SDL_SCANCODE_RCTRL:              return "\\{keyboard r_ctrl}";
        case SDL_SCANCODE_RSHIFT:             return "\\{keyboard r_shift}";
        case SDL_SCANCODE_LSHIFT:             return "\\{keyboard l_shift}";
        case SDL_SCANCODE_LALT:               return "\\{keyboard l_alt}";
        case SDL_SCANCODE_RALT:               return "\\{keyboard r_alt}";
        case SDL_SCANCODE_LGUI:               return "\\{keyboard l_win}";
        case SDL_SCANCODE_RGUI:               return "\\{keyboard r_win}";

        case SDL_SCANCODE_LEFT:               return "\\{keyboard left}";
        case SDL_SCANCODE_UP:                 return "\\{keyboard up}";
        case SDL_SCANCODE_RIGHT:              return "\\{keyboard right}";
        case SDL_SCANCODE_DOWN:               return "\\{keyboard down}";

        case SDL_SCANCODE_RETURN:             return "\\{keyboard return}";
        case SDL_SCANCODE_ESCAPE:             return "\\{keyboard escape}";
        case SDL_SCANCODE_BACKSPACE:          return "\\{keyboard backspace}";
        case SDL_SCANCODE_TAB:                return "\\{keyboard tab}";
        case SDL_SCANCODE_SPACE:              return "\\{keyboard space}";
        case SDL_SCANCODE_CAPSLOCK:           return "\\{keyboard caps_lock}";
        case SDL_SCANCODE_PRINTSCREEN:        return "\\{keyboard print_screen}";
        case SDL_SCANCODE_SCROLLLOCK:         return "\\{keyboard scroll_lock}";
        case SDL_SCANCODE_PAUSE:              return "\\{keyboard pause}";
        case SDL_SCANCODE_INSERT:             return "\\{keyboard insert}";
        case SDL_SCANCODE_HOME:               return "\\{keyboard home}";
        case SDL_SCANCODE_PAGEUP:             return "\\{keyboard page_up}";
        case SDL_SCANCODE_DELETE:             return "\\{keyboard delete}";
        case SDL_SCANCODE_END:                return "\\{keyboard end}";
        case SDL_SCANCODE_PAGEDOWN:           return "\\{keyboard page_down}";

        case SDL_SCANCODE_A:                  return "\\{keyboard a}";
        case SDL_SCANCODE_B:                  return "\\{keyboard b}";
        case SDL_SCANCODE_C:                  return "\\{keyboard c}";
        case SDL_SCANCODE_D:                  return "\\{keyboard d}";
        case SDL_SCANCODE_E:                  return "\\{keyboard e}";
        case SDL_SCANCODE_F:                  return "\\{keyboard f}";
        case SDL_SCANCODE_G:                  return "\\{keyboard g}";
        case SDL_SCANCODE_H:                  return "\\{keyboard h}";
        case SDL_SCANCODE_I:                  return "\\{keyboard i}";
        case SDL_SCANCODE_J:                  return "\\{keyboard j}";
        case SDL_SCANCODE_K:                  return "\\{keyboard k}";
        case SDL_SCANCODE_L:                  return "\\{keyboard l}";
        case SDL_SCANCODE_M:                  return "\\{keyboard m}";
        case SDL_SCANCODE_N:                  return "\\{keyboard n}";
        case SDL_SCANCODE_O:                  return "\\{keyboard o}";
        case SDL_SCANCODE_P:                  return "\\{keyboard p}";
        case SDL_SCANCODE_Q:                  return "\\{keyboard q}";
        case SDL_SCANCODE_R:                  return "\\{keyboard r}";
        case SDL_SCANCODE_S:                  return "\\{keyboard s}";
        case SDL_SCANCODE_T:                  return "\\{keyboard t}";
        case SDL_SCANCODE_U:                  return "\\{keyboard u}";
        case SDL_SCANCODE_V:                  return "\\{keyboard v}";
        case SDL_SCANCODE_W:                  return "\\{keyboard w}";
        case SDL_SCANCODE_X:                  return "\\{keyboard x}";
        case SDL_SCANCODE_Y:                  return "\\{keyboard y}";
        case SDL_SCANCODE_Z:                  return "\\{keyboard z}";

        case SDL_SCANCODE_0:                  return "\\{keyboard 0}";
        case SDL_SCANCODE_1:                  return "\\{keyboard 1}";
        case SDL_SCANCODE_2:                  return "\\{keyboard 2}";
        case SDL_SCANCODE_3:                  return "\\{keyboard 3}";
        case SDL_SCANCODE_4:                  return "\\{keyboard 4}";
        case SDL_SCANCODE_5:                  return "\\{keyboard 5}";
        case SDL_SCANCODE_6:                  return "\\{keyboard 6}";
        case SDL_SCANCODE_7:                  return "\\{keyboard 7}";
        case SDL_SCANCODE_8:                  return "\\{keyboard 8}";
        case SDL_SCANCODE_9:                  return "\\{keyboard 9}";

        case SDL_SCANCODE_MINUS:              return "\\{keyboard minus}";
        case SDL_SCANCODE_EQUALS:             return "\\{keyboard equals}";
        case SDL_SCANCODE_LEFTBRACKET:        return "\\{keyboard left_square_bracket}";
        case SDL_SCANCODE_RIGHTBRACKET:       return "\\{keyboard right_square_bracket}";
        case SDL_SCANCODE_BACKSLASH:          return "\\{keyboard backslash}";
        case SDL_SCANCODE_NONUSHASH:          return "\\{keyboard hash}";
        case SDL_SCANCODE_SEMICOLON:          return "\\{keyboard semicolon}";
        case SDL_SCANCODE_APOSTROPHE:         return "\\{keyboard apostrophe}";
        case SDL_SCANCODE_GRAVE:              return "\\{keyboard backtick}";
        case SDL_SCANCODE_COMMA:              return "\\{keyboard comma}";
        case SDL_SCANCODE_PERIOD:             return "\\{keyboard period}";
        case SDL_SCANCODE_SLASH:              return "\\{keyboard slash}";
        case SDL_SCANCODE_NONUSBACKSLASH:     return "\\{keyboard backslash}";

        case SDL_SCANCODE_F1:                 return "\\{keyboard f1}";
        case SDL_SCANCODE_F2:                 return "\\{keyboard f2}";
        case SDL_SCANCODE_F3:                 return "\\{keyboard f3}";
        case SDL_SCANCODE_F4:                 return "\\{keyboard f4}";
        case SDL_SCANCODE_F5:                 return "\\{keyboard f5}";
        case SDL_SCANCODE_F6:                 return "\\{keyboard f6}";
        case SDL_SCANCODE_F7:                 return "\\{keyboard f7}";
        case SDL_SCANCODE_F8:                 return "\\{keyboard f8}";
        case SDL_SCANCODE_F9:                 return "\\{keyboard f9}";
        case SDL_SCANCODE_F10:                return "\\{keyboard f10}";
        case SDL_SCANCODE_F11:                return "\\{keyboard f11}";
        case SDL_SCANCODE_F12:                return "\\{keyboard f12}";
        case SDL_SCANCODE_F13:                return "\\{keyboard f13}";
        case SDL_SCANCODE_F14:                return "\\{keyboard f14}";
        case SDL_SCANCODE_F15:                return "\\{keyboard f15}";
        case SDL_SCANCODE_F16:                return "\\{keyboard f16}";
        case SDL_SCANCODE_F17:                return "\\{keyboard f17}";
        case SDL_SCANCODE_F18:                return "\\{keyboard f18}";
        case SDL_SCANCODE_F19:                return "\\{keyboard f19}";
        case SDL_SCANCODE_F20:                return "\\{keyboard f20}";
        case SDL_SCANCODE_F21:                return "\\{keyboard f21}";
        case SDL_SCANCODE_F22:                return "\\{keyboard f22}";
        case SDL_SCANCODE_F23:                return "\\{keyboard f23}";
        case SDL_SCANCODE_F24:                return "\\{keyboard f24}";

        case SDL_SCANCODE_NUMLOCKCLEAR:       return "\\{keyboard num_lock}";
        case SDL_SCANCODE_KP_0:               return "\\{keyboard num_0}";
        case SDL_SCANCODE_KP_1:               return "\\{keyboard num_1}";
        case SDL_SCANCODE_KP_2:               return "\\{keyboard num_2}";
        case SDL_SCANCODE_KP_3:               return "\\{keyboard num_3}";
        case SDL_SCANCODE_KP_4:               return "\\{keyboard num_4}";
        case SDL_SCANCODE_KP_5:               return "\\{keyboard num_5}";
        case SDL_SCANCODE_KP_6:               return "\\{keyboard num_6}";
        case SDL_SCANCODE_KP_7:               return "\\{keyboard num_7}";
        case SDL_SCANCODE_KP_8:               return "\\{keyboard num_8}";
        case SDL_SCANCODE_KP_9:               return "\\{keyboard num_9}";
        case SDL_SCANCODE_KP_PERIOD:          return "\\{keyboard num_period}";
        case SDL_SCANCODE_KP_DIVIDE:          return "\\{keyboard num_divide}";
        case SDL_SCANCODE_KP_MULTIPLY:        return "\\{keyboard num_multiply}";
        case SDL_SCANCODE_KP_MINUS:           return "\\{keyboard num_minus}";
        case SDL_SCANCODE_KP_PLUS:            return "\\{keyboard num_plus}";
        case SDL_SCANCODE_KP_EQUALS:          return "\\{keyboard num_equals}";
        case SDL_SCANCODE_KP_EQUALSAS400:     return "\\{keyboard num_equals}";
        case SDL_SCANCODE_KP_COMMA:           return "\\{keyboard num_comma}";
        case SDL_SCANCODE_KP_ENTER:           return "\\{keyboard num_enter}";

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
        case SDL_SCANCODE_UNKNOWN:            return nullptr;

        default:                              return "\\{keyboard unknown}";
    }
    // clang-format on
}

static bool M_CheckScancode(const SDL_Scancode scancode, const bool exact)
{
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return false;
    }
#ifdef _WIN32
    if (scancode == SDL_SCANCODE_F4
        && (KEY_DOWN(SDL_SCANCODE_LALT) || KEY_DOWN(SDL_SCANCODE_RALT))) {
        return false;
    }
#endif
    if (KEY_DOWN(scancode)) {
        return true;
    }
    if (exact) {
        return false;
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

// A binding of one key takes either side of a paired modifier, so that Ctrl
// bound as action answers to both. A combo takes the side it was bound to:
// Alt+Enter otherwise fires from the Alt a player jumps with.
static bool M_CheckBinding(const KEYBOARD_BINDING *const bind)
{
    if (bind->key_count == 0) {
        return false;
    }
    for (int32_t k = 0; k < bind->key_count; k++) {
        if (!M_CheckScancode(bind->keys[k], bind->key_count > 1)) {
            return false;
        }
    }
    return true;
}

static const KEYBOARD_BINDING *M_GetBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    return &m_Layout[layout][role].slots[slot];
}

// Combo adapter functions for the shared combo layer.
static INPUT_COMBO_BINDING M_GetComboBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const KEYBOARD_BINDING *b = M_GetBinding(layout, role, slot);
    return (INPUT_COMBO_BINDING) {
        .key_count = b->key_count,
        .keys = b->keys,
        .key_stride = sizeof(SDL_Scancode),
    };
}

static bool M_ComboKeysEqual(const void *const a, const void *const b)
{
    return *(const SDL_Scancode *)a == *(const SDL_Scancode *)b;
}

static bool M_Key(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
        const KEYBOARD_BINDING *bind = &m_Layout[layout][role].slots[slot];
        if (bind->key_count >= 2
            && Input_ComboIsKeyImmediate(
                layout, &bind->keys[0], M_GetComboBinding, M_ComboKeysEqual)) {
            continue;
        }
        if (M_CheckBinding(bind)) {
            return true;
        }
    }
    return false;
}

static bool M_BindingsEqual(
    const KEYBOARD_BINDING *const a, const KEYBOARD_BINDING *const b)
{
    if (a->key_count != b->key_count || a->key_count == 0) {
        return false;
    }
    for (int32_t i = 0; i < a->key_count; i++) {
        if (a->keys[i] != b->keys[i]) {
            return false;
        }
    }
    return true;
}

static bool M_CheckConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role1, const INPUT_ROLE role2)
{
    for (int32_t s1 = 0; s1 < INPUT_BINDING_SLOTS; s1++) {
        const KEYBOARD_BINDING *b1 = M_GetBinding(layout, role1, s1);
        if (b1->key_count == 0) {
            continue;
        }
        for (int32_t s2 = 0; s2 < INPUT_BINDING_SLOTS; s2++) {
            const KEYBOARD_BINDING *b2 = M_GetBinding(layout, role2, s2);
            if (M_BindingsEqual(b1, b2)) {
                return true;
            }
        }
    }
    return false;
}

static void M_AssignConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, bool conflict)
{
    m_Conflicts[layout][role] = conflict;
}

static void M_CheckConflicts(const INPUT_LAYOUT layout)
{
    Input_ConflictHelper(layout, M_CheckConflict, M_AssignConflict);
    Input_ComboCheckConflicts(
        layout, M_GetComboBinding, M_ComboKeysEqual, m_Conflicts[layout]);
}

static void M_AssignBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const KEYBOARD_BINDING *const bind)
{
    m_Layout[layout][role].slots[slot] = *bind;
    M_CheckConflicts(layout);
}

static void M_ResetLayout(const INPUT_LAYOUT layout)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        m_Layout[layout][role] = m_Layout[INPUT_LAYOUT_DEFAULT][role];
    }
    M_CheckConflicts(layout);
}

static BUILTIN_KEYBOARD_LAYOUT *M_GetBuiltInLayout(const INPUT_ROLE role)
{
    for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
        BUILTIN_KEYBOARD_LAYOUT *const builtin = &m_BuiltinLayout[i];
        if (builtin->role == role) {
            return builtin;
        }
    }
    return nullptr;
}

static void M_HandleBuiltInDefaults(void)
{
#define L_BIND(role, code)                                                     \
    do {                                                                       \
        M_GetBuiltInLayout(role)->bind = (KEYBOARD_BINDING) {                  \
            .key_count = (code) != SDL_SCANCODE_UNKNOWN ? 1 : 0,               \
            .keys = { code },                                                  \
        };                                                                     \
    } while (0)

    if (g_TRVersion == 2) {
        L_BIND(INPUT_ROLE_EQUIP_MAGNUMS, SDL_SCANCODE_UNKNOWN);
        L_BIND(INPUT_ROLE_EQUIP_AUTOS, SDL_SCANCODE_3);
    } else if (g_TRVersion == 3) {
        L_BIND(INPUT_ROLE_USE_SMALL_MEDI, SDL_SCANCODE_0);
        L_BIND(INPUT_ROLE_USE_BIG_MEDI, SDL_SCANCODE_9);
        L_BIND(INPUT_ROLE_EQUIP_MAGNUMS, SDL_SCANCODE_UNKNOWN);
        L_BIND(INPUT_ROLE_EQUIP_DESERT_EAGLE, SDL_SCANCODE_3);
        L_BIND(INPUT_ROLE_EQUIP_M16, SDL_SCANCODE_UNKNOWN);
        L_BIND(INPUT_ROLE_EQUIP_MP5, SDL_SCANCODE_6);
        L_BIND(INPUT_ROLE_EQUIP_ROCKET_LAUNCHER, SDL_SCANCODE_7);
        L_BIND(INPUT_ROLE_EQUIP_GRENADE_LAUNCHER, SDL_SCANCODE_8);
    }

#undef L_BIND
}

static void M_Init(void)
{
    memcpy(m_BuiltinLayout, m_BuiltinLayoutBase, sizeof(m_BuiltinLayout));

    // first, reset all roles to unbound
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
            m_Layout[INPUT_LAYOUT_DEFAULT][role].slots[slot] =
                (KEYBOARD_BINDING) { .key_count = 0 };
        }
    }
    // allow specific engines to re-assign default bindings
    M_HandleBuiltInDefaults();
    // then load the defined default bindings into slot 0
    for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
        const BUILTIN_KEYBOARD_LAYOUT *const builtin = &m_BuiltinLayout[i];
        m_Layout[INPUT_LAYOUT_DEFAULT][builtin->role].slots[0] = builtin->bind;
    }
    M_CheckConflicts(INPUT_LAYOUT_DEFAULT);

    for (int32_t layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_ResetLayout(layout);
    }
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    // we only do this for keyboard input
    // Skip reads confirm before the line below folds action into it, so that
    // skip names the keys it answers to rather than inheriting them.
    result->menu_skip |= result->action || result->menu_confirm;
    result->menu_confirm |= result->action;
    result->menu_show_info |= result->look;
    result->menu_fine_adjust |= result->slow;
    result->menu_coarse_adjust |= result->draw;
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

static const char *M_GetName(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    INPUT_ROLE actual_role = role;
    switch (role) {
    case INPUT_ROLE_MENU_CONFIRM:
        actual_role = INPUT_ROLE_ACTION;
        break;
    case INPUT_ROLE_MENU_SHOW_INFO:
        actual_role = INPUT_ROLE_LOOK;
        break;
    case INPUT_ROLE_MENU_FINE_ADJUST:
        actual_role = INPUT_ROLE_SLOW;
        break;
    case INPUT_ROLE_MENU_COARSE_ADJUST:
        actual_role = INPUT_ROLE_DRAW_WEAPON;
        break;
    case INPUT_ROLE_NUMBER_OF:
        break;
    default:
        break;
    }

    const KEYBOARD_BINDING *const bind =
        M_GetBinding(layout, actual_role, slot);
    const char *names[INPUT_COMBO_MAX_KEYS];
    for (int32_t k = 0; k < bind->key_count; k++) {
        names[k] = M_GetScancodeName(bind->keys[k]);
    }
    return Input_JoinKeyNames(names, bind->key_count);
}

static void M_UnassignRole(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const KEYBOARD_BINDING empty = { .key_count = 0 };
    M_AssignBinding(layout, role, slot, &empty);
}

static bool M_AssignFromJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const JSON_OBJECT *const bind_obj)
{
    const JSON_ARRAY *const combo_arr = JSON_ObjectGetArray(bind_obj, "combo");
    if (combo_arr != nullptr) {
        // New combo format: "combo": [scancode1, scancode2, ...]
        const int32_t count = combo_arr->length < INPUT_COMBO_MAX_KEYS
            ? (int32_t)combo_arr->length
            : INPUT_COMBO_MAX_KEYS;
        KEYBOARD_BINDING bind = { .key_count = count };
        for (int32_t i = 0; i < count; i++) {
            bind.keys[i] = JSON_ArrayGetInt(combo_arr, i, SDL_SCANCODE_UNKNOWN);
        }
        M_AssignBinding(layout, role, slot, &bind);
    } else {
        // Legacy single-key format: "scancode": N
        const KEYBOARD_BINDING *current = M_GetBinding(layout, role, slot);
        const SDL_Scancode default_sc =
            current->key_count > 0 ? current->keys[0] : SDL_SCANCODE_UNKNOWN;
        const SDL_Scancode user_sc =
            JSON_ObjectGetInt(bind_obj, "scancode", default_sc);
        const KEYBOARD_BINDING bind = {
            .key_count = user_sc != SDL_SCANCODE_UNKNOWN ? 1 : 0,
            .keys = { user_sc },
        };
        M_AssignBinding(layout, role, slot, &bind);
    }
    return true;
}

static bool M_AssignToJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    JSON_OBJECT *const bind_obj)
{
    const KEYBOARD_BINDING *user = M_GetBinding(layout, role, slot);
    const KEYBOARD_BINDING *def =
        M_GetBinding(INPUT_LAYOUT_DEFAULT, role, slot);

    if (M_BindingsEqual(user, def)
        || (user->key_count == 0 && def->key_count == 0)) {
        return false;
    }

    if (user->key_count <= 1) {
        // Single key: use legacy "scancode" for backward compatibility
        JSON_ObjectAppendInt(
            bind_obj, "scancode",
            user->key_count == 1 ? user->keys[0] : SDL_SCANCODE_UNKNOWN);
    } else {
        // Multi-key combo
        JSON_ARRAY *const arr = JSON_ArrayNew();
        for (int32_t i = 0; i < user->key_count; i++) {
            JSON_ArrayAppendInt(arr, user->keys[i]);
        }
        JSON_ObjectAppendArray(bind_obj, "combo", arr);
    }
    return true;
}

static uint32_t M_GetPressTick(const void *const key)
{
    return m_KeyDownTick[*(const SDL_Scancode *)key];
}

static INPUT_COMBO_BINDING M_ToCombo(const KEYBOARD_BINDING *const b)
{
    return (INPUT_COMBO_BINDING) {
        .key_count = b->key_count,
        .keys = b->keys,
        .key_stride = sizeof(SDL_Scancode),
    };
}

static const KEYBOARD_BINDING *M_GetPressedBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
        const KEYBOARD_BINDING *bind = M_GetBinding(layout, role, slot);
        if (M_CheckBinding(bind)) {
            return bind;
        }
    }
    return nullptr;
}

static void M_ResolveCombos(
    const INPUT_LAYOUT layout, INPUT_STATE *const result)
{
    // Update press-tick tracking: record the tick each key most recently
    // transitioned from released to held.
    m_Tick++;
    for (SDL_Scancode sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        if (KEY_DOWN(sc) && !m_PrefixWasHeld[sc]) {
            m_KeyDownTick[sc] = m_Tick;
        }
    }

    // Phase 1: Collect active bindings.
    const KEYBOARD_BINDING *active[INPUT_ROLE_NUMBER_OF] = {};
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (InputState_GetRole(*result, role)) {
            active[role] = M_GetPressedBinding(layout, role);
        }
    }

    // Suppress invalid combos (non-capturing sustained + immediate).
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] != nullptr
            && Input_ComboSustainedHasImmediate(
                layout, M_ToCombo(active[role]), M_GetComboBinding,
                M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            active[role] = nullptr;
        }
    }

    // Suppress combos that appear to ride on top of ongoing movement:
    // if the earliest-pressed key in the combo is bound to an immediate
    // role, the user was already performing that action when they pressed
    // the remaining combo keys.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] != nullptr
            && Input_ComboEarliestKeyIsImmediate(
                layout, M_ToCombo(active[role]), M_GetPressTick,
                M_GetComboBinding, M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            active[role] = nullptr;
        }
    }

    // Phase 2: Subset suppression — longer active combos suppress shorter.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr) {
            continue;
        }
        for (INPUT_ROLE other = 0; other < INPUT_ROLE_NUMBER_OF; other++) {
            if (other == role || active[other] == nullptr) {
                continue;
            }
            if (Input_ComboIsProperSubset(
                    M_ToCombo(active[role]), M_ToCombo(active[other]),
                    M_ComboKeysEqual)) {
                InputState_ClearRole(result, role);
                break;
            }
        }
    }

    // Phase 3: Combo deferral — if an active combo's binding is a proper
    // subset of some (not necessarily active) longer binding, defer it.
    // This handles both single-key and multi-key prefix disambiguation.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr) {
            continue;
        }
        if (((Input_IsRoleImmediate(role) || Input_IsRoleSustained(role))
             && active[role]->key_count <= 1)
            || !Input_IsRoleRebindable(role)) {
            continue;
        }
        if (Input_ComboHasLonger(
                layout, role, M_ToCombo(active[role]), M_GetComboBinding,
                M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            if (!m_RoleWasActive[role]) {
                m_RoleLongerFired[role] = false;
            }
            m_RoleWasActive[role] = true;
        }
    }

    // Reset prefix tracking for newly pressed keys.
    for (SDL_Scancode sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        if (KEY_DOWN(sc) && !m_PrefixWasHeld[sc]) {
            m_PrefixComboFired[sc] = false;
        }
    }

    // Phase 4: Mark longer-combo-fired state.
    // When a combo fires, mark all shorter deferred combos as superseded.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr || active[role]->key_count < 2) {
            continue;
        }
        // Mark scancodes for single-key prefix deferral.
        for (int32_t k = 0; k < active[role]->key_count; k++) {
            m_PrefixComboFired[active[role]->keys[k]] = true;
        }
        // Mark shorter deferred roles as superseded.
        for (INPUT_ROLE other = 0; other < INPUT_ROLE_NUMBER_OF; other++) {
            if (!m_RoleWasActive[other]) {
                continue;
            }
            const KEYBOARD_BINDING *ob = M_GetPressedBinding(layout, other);
            if (ob != nullptr
                && Input_ComboIsProperSubset(
                    M_ToCombo(ob), M_ToCombo(active[role]), M_ComboKeysEqual)) {
                m_RoleLongerFired[other] = true;
            }
        }
    }

    // Phase 5: Fire deferred roles on release.
    // When a deferred role's binding is no longer fully pressed and no
    // longer combo fired, inject the role for one frame.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (!m_RoleWasActive[role]) {
            continue;
        }
        const KEYBOARD_BINDING *bind = M_GetPressedBinding(layout, role);
        if (bind != nullptr) {
            continue;
        }
        if (!m_RoleLongerFired[role]) {
            InputState_SetRole(result, role, true);
        }
        m_RoleWasActive[role] = false;
        m_RoleLongerFired[role] = false;
    }

    // Phase 6: Single-key prefix deferral.
    for (SDL_Scancode sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        const bool held = KEY_DOWN(sc);

        if (held
            && Input_ComboIsStarter(
                layout, &sc, M_GetComboBinding, M_ComboKeysEqual)) {
            const INPUT_ROLE role = Input_ComboFindDeferrableRole(
                layout, &sc, M_GetComboBinding, M_ComboKeysEqual);
            if (role != (INPUT_ROLE)-1) {
                InputState_ClearRole(result, role);
            }
        }

        if (!held && m_PrefixWasHeld[sc] && !m_PrefixComboFired[sc]) {
            const INPUT_ROLE role = Input_ComboFindDeferrableRole(
                layout, &sc, M_GetComboBinding, M_ComboKeysEqual);
            if (role != (INPUT_ROLE)-1) {
                InputState_SetRole(result, role, true);
            }
        }

        m_PrefixWasHeld[sc] = held;
    }
}

static bool M_CaptureHasKey(const SDL_Scancode scancode)
{
    for (int32_t i = 0; i < m_CaptureBuffer.key_count; i++) {
        if (m_CaptureBuffer.keys[i] == scancode) {
            return true;
        }
    }
    return false;
}

static bool M_ReadAndAssign(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    // Count currently held keys and accumulate new ones into the buffer.
    bool any_held = false;
    for (SDL_Scancode sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        if (!KEY_DOWN(sc)) {
            continue;
        }
        any_held = true;
        if (!M_CaptureHasKey(sc)
            && m_CaptureBuffer.key_count < INPUT_COMBO_MAX_KEYS) {
            m_CaptureBuffer.keys[m_CaptureBuffer.key_count++] = sc;
        }
        m_CaptureActive = true;
    }

    // If the first key captured is bound to an immediate role (movement,
    // action, etc.), assign as single key right away — don't wait for combo.
    if (m_CaptureActive && m_CaptureBuffer.key_count == 1 && any_held
        && Input_ComboIsKeyImmediate(
            layout, &m_CaptureBuffer.keys[0], M_GetComboBinding,
            M_ComboKeysEqual)) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.key_count = 0;
        m_CaptureActive = false;
        return true;
    }

    // All keys released after at least one was captured — assign the chord.
    if (!any_held && m_CaptureActive) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.key_count = 0;
        m_CaptureActive = false;
        return true;
    }
    return false;
}

INPUT_BACKEND_IMPL g_Input_Keyboard = {
    .init = M_Init,
    .shutdown = nullptr,
    .discover = nullptr,
    .process_event = M_ProcessEvent,
    .custom_update = M_CustomUpdate,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = M_IsRoleConflicted,
    .get_name = M_GetName,
    .unassign_role = M_UnassignRole,
    .assign_from_json_object = M_AssignFromJSONObject,
    .assign_to_json_object = M_AssignToJSONObject,
    .reset_layout = M_ResetLayout,
    .read_and_assign = M_ReadAndAssign,
    .resolve_combos = M_ResolveCombos,
};

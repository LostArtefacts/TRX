#include "game/input.h"

#include "game/console/common.h"
#include "global/funcs.h"
#include "global/vars.h"
#include "specific/s_input.h"

static const char *m_KeyNames[] = {
    NULL,   "ESC",   "1",     "2",     "3",     "4",     "5",     "6",
    "7",    "8",     "9",     "0",     "-",     "+",     "BKSP",  "TAB",
    "Q",    "W",     "E",     "R",     "T",     "Y",     "U",     "I",
    "O",    "P",     "<",     ">",     "RET",   "CTRL",  "A",     "S",
    "D",    "F",     "G",     "H",     "J",     "K",     "L",     ";",
    "'",    "`",     "SHIFT", "#",     "Z",     "X",     "C",     "V",
    "B",    "N",     "M",     ",",     ".",     "/",     "SHIFT", "PADx",
    "ALT",  "SPACE", "CAPS",  NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    "NMLK",  NULL,    "PAD7",
    "PAD8", "PAD9",  "PAD-",  "PAD4",  "PAD5",  "PAD6",  "PAD+",  "PAD1",
    "PAD2", "PAD3",  "PAD0",  "PAD.",  NULL,    NULL,    "\\",    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    "ENTER", "CTRL",  NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    "SHIFT", NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    "PAD/",  NULL,    NULL,
    "ALT",  NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    "HOME",
    "UP",   "PGUP",  NULL,    "LEFT",  NULL,    "RIGHT", NULL,    "END",
    "DOWN", "PGDN",  "INS",   "DEL",   NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    NULL,   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,
    "JOY1", "JOY2",  "JOY3",  "JOY4",  "JOY5",  "JOY6",  "JOY7",  "JOY8",
    "JOY9", "JOY10", "JOY11", "JOY12", "JOY13", "JOY14", "JOY15", "JOY16",
};

bool g_ConflictLayout[INPUT_ROLE_NUMBER_OF] = { false };
bool m_ListenMode = false;

bool Input_Update(void)
{
    bool result = S_Input_Update();

    g_InputDB = Input_GetDebounced(g_Input);

    if (m_ListenMode) {
        g_Input = 0;
        g_InputDB = 0;
        return result;
    }

    if (g_InputDB & IN_CONSOLE) {
        Console_Open();
        g_Input = 0;
        g_InputDB = 0;
    }

    return result;
}

int32_t __cdecl Input_GetDebounced(const int32_t input)
{
    const int32_t result = input & ~g_OldInputDB;
    g_OldInputDB = input;
    return result;
}

uint16_t Input_GetAssignedKey(const int32_t layout, const INPUT_ROLE role)
{
    return g_Layout[layout].key[role];
}

void Input_AssignKey(
    const int32_t layout, const INPUT_ROLE role, const uint16_t key)
{
    g_Layout[layout].key[role] = key;
}

const char *Input_GetLayoutName(const int32_t layout)
{
    // clang-format off
    switch (layout) {
    case 0: return g_GF_PCStrings[GF_S_PC_DEFAULT_KEYS];
    case 1: return g_GF_PCStrings[GF_S_PC_USER_KEYS];
    default: return "";
    }
    // clang-format on
}

const char *Input_GetRoleName(const INPUT_ROLE role)
{
    // clang-format off
    switch (role) {
    case INPUT_ROLE_UP:          return g_GF_GameStrings[GF_S_GAME_KEYMAP_RUN];
    case INPUT_ROLE_DOWN:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_BACK];
    case INPUT_ROLE_LEFT:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_LEFT];
    case INPUT_ROLE_RIGHT:       return g_GF_GameStrings[GF_S_GAME_KEYMAP_RIGHT];
    case INPUT_ROLE_STEP_LEFT:   return g_GF_GameStrings[GF_S_GAME_KEYMAP_STEP_LEFT];
    case INPUT_ROLE_STEP_RIGHT:  return g_GF_GameStrings[GF_S_GAME_KEYMAP_STEP_RIGHT];
    case INPUT_ROLE_SLOW:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_WALK];
    case INPUT_ROLE_JUMP:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_JUMP];
    case INPUT_ROLE_ACTION:      return g_GF_GameStrings[GF_S_GAME_KEYMAP_ACTION];
    case INPUT_ROLE_DRAW_WEAPON: return g_GF_GameStrings[GF_S_GAME_KEYMAP_DRAW_WEAPON];
    case INPUT_ROLE_FLARE:       return g_GF_GameStrings[GF_S_GAME_KEYMAP_FLARE];
    case INPUT_ROLE_LOOK:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_LOOK];
    case INPUT_ROLE_ROLL:        return g_GF_GameStrings[GF_S_GAME_KEYMAP_ROLL];
    case INPUT_ROLE_OPTION:      return g_GF_GameStrings[GF_S_GAME_KEYMAP_INVENTORY];
    case INPUT_ROLE_CONSOLE:     return "Console";
    default:                     return "";
    }
    // clang-format on
}

const char *Input_GetKeyName(const uint16_t key)
{
    return m_KeyNames[key];
}

void __cdecl Input_CheckConflictsWithDefaults(void)
{
    for (int32_t i = 0; i < INPUT_ROLE_NUMBER_OF; i++) {
        g_ConflictLayout[i] = false;
        for (int32_t j = 0; j < INPUT_ROLE_NUMBER_OF; j++) {
            const uint16_t key1 = Input_GetAssignedKey(0, i);
            const uint16_t key2 = Input_GetAssignedKey(1, j);
            if (key1 == key2) {
                g_ConflictLayout[i] = true;
                break;
            }
        }
    }
}

void Input_EnterListenMode(void)
{
    m_ListenMode = true;
}

void Input_ExitListenMode(void)
{
    m_ListenMode = false;
    S_Input_Update();
    g_OldInputDB = g_Input;
    g_InputDB = Input_GetDebounced(g_Input);
}

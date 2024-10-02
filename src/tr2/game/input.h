#pragma once

#include "global/types.h"

#define INPUT_MAX_LAYOUT 2

typedef enum {
    // clang-format off
    INPUT_ROLE_UP          = 0,
    INPUT_ROLE_FORWARD     = INPUT_ROLE_UP,
    INPUT_ROLE_DOWN        = 1,
    INPUT_ROLE_BACK        = INPUT_ROLE_DOWN,
    INPUT_ROLE_LEFT        = 2,
    INPUT_ROLE_RIGHT       = 3,
    INPUT_ROLE_STEP_LEFT   = 4,
    INPUT_ROLE_STEP_RIGHT  = 5,
    INPUT_ROLE_SLOW        = 6,
    INPUT_ROLE_JUMP        = 7,
    INPUT_ROLE_ACTION      = 8,
    INPUT_ROLE_DRAW_WEAPON = 9,
    INPUT_ROLE_FLARE       = 10,
    INPUT_ROLE_LOOK        = 11,
    INPUT_ROLE_ROLL        = 12,
    INPUT_ROLE_OPTION      = 13,
    INPUT_ROLE_CONSOLE     = 14,
    INPUT_ROLE_NUMBER_OF   = 15,
    // clang-format on
} INPUT_ROLE;

typedef enum {
    // clang-format off
    IN_FORWARD     = 1 << 0,
    IN_BACK        = 1 << 1,
    IN_LEFT        = 1 << 2,
    IN_RIGHT       = 1 << 3,
    IN_JUMP        = 1 << 4,
    IN_DRAW        = 1 << 5,
    IN_ACTION      = 1 << 6,
    IN_SLOW        = 1 << 7,
    IN_OPTION      = 1 << 8,
    IN_LOOK        = 1 << 9,
    IN_STEP_LEFT   = 1 << 10,
    IN_STEP_RIGHT  = 1 << 11,
    IN_ROLL        = 1 << 12,
    IN_PAUSE       = 1 << 13,
    IN_RESERVED1   = 1 << 14,
    IN_RESERVED2   = 1 << 15,
    IN_DOZY_CHEAT  = 1 << 16,
    IN_STUFF_CHEAT = 1 << 17,
    IN_DEBUG_INFO  = 1 << 18,
    IN_FLARE       = 1 << 19,
    IN_SELECT      = 1 << 20,
    IN_DESELECT    = 1 << 21,
    IN_SAVE        = 1 << 22,
    IN_LOAD        = 1 << 23,
    IN_CONSOLE     = 1 << 24,
    // clang-format on
} INPUT_STATE;

typedef struct {
    uint16_t key[INPUT_ROLE_NUMBER_OF];
} INPUT_LAYOUT;

extern INPUT_LAYOUT g_Layout[2];
extern bool g_ConflictLayout[INPUT_ROLE_NUMBER_OF];

bool Input_Update(void);
int32_t __cdecl Input_GetDebounced(int32_t input);

void Input_EnterListenMode(void);
void Input_ExitListenMode(void);
bool Input_IsAnythingPressed(void);
void Input_AssignKey(int32_t layout, INPUT_ROLE role, uint16_t key);
uint16_t Input_GetAssignedKey(int32_t layout, INPUT_ROLE role);

const char *Input_GetLayoutName(int32_t layout);
const char *Input_GetRoleName(INPUT_ROLE role);
const char *Input_GetKeyName(uint16_t key);

void __cdecl Input_CheckConflictsWithDefaults(void);

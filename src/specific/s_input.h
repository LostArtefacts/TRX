#ifndef T1M_SPECIFIC_S_INPUT_H
#define T1M_SPECIFIC_S_INPUT_H

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef int16_t S_INPUT_KEYCODE;

void S_Input_Init();

INPUT_STATE S_Input_GetCurrentState();

S_INPUT_KEYCODE S_Input_ReadKeyCode();

const char *S_Input_GetKeyCodeName(S_INPUT_KEYCODE key);

bool S_Input_IsKeyConflicted(INPUT_KEY key);
void S_Input_SetKeyAsConflicted(INPUT_KEY key, bool is_conflicted);

S_INPUT_KEYCODE S_Input_GetAssignedKeyCode(int16_t layout_num, INPUT_KEY key);
void S_Input_AssignKeyCode(
    int16_t layout_num, INPUT_KEY key, S_INPUT_KEYCODE key_code);

#endif

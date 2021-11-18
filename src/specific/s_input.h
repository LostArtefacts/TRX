#ifndef T1M_SPECIFIC_S_INPUT_H
#define T1M_SPECIFIC_S_INPUT_H

#include "global/types.h"

#include <stdbool.h>

void S_Input_Init();

INPUT_STATE S_Input_GetCurrentState();

int16_t S_Input_ReadKeyCode();

const char *S_Input_GetKeyCodeName(int16_t key);

bool S_Input_IsKeyConflicted(INPUT_KEY key);
void S_Input_SetKeyAsConflicted(INPUT_KEY key, bool is_conflicted);

int16_t S_Input_GetAssignedKeyCode(int16_t layout_num, INPUT_KEY key);
void S_Input_AssignKeyCode(int16_t layout_num, INPUT_KEY key, int16_t key_code);

#endif

#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void S_Input_Init(void);
void S_Input_Shutdown(void);

INPUT_STATE S_Input_GetCurrentState(void);

INPUT_SCANCODE S_Input_ReadScancode(void);

const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode);

bool S_Input_IsKeyConflicted(INPUT_ROLE role);
void S_Input_SetKeyAsConflicted(INPUT_ROLE role, bool is_conflicted);

INPUT_SCANCODE S_Input_GetAssignedScancode(int16_t layout_num, INPUT_ROLE role);
void S_Input_AssignScancode(
    int16_t layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode);

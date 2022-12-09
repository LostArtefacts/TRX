#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;
extern bool g_ConflictLayout[INPUT_ROLE_NUMBER_OF];

void Input_Init(void);
void Input_Shutdown(void);
void Input_Update(void);

// Checks the current layout for conflicts with other layouts.
void Input_CheckConflicts(INPUT_LAYOUT layout_num);

// Returns whether the key assigned to the given role is also used elsewhere
// within the custom layout.
bool Input_IsKeyConflictedWithUser(INPUT_ROLE role);

// Returns whether the key assigned to the given role is conflicting with a
// default key binding.
bool Input_IsKeyConflictedWithDefault(INPUT_ROLE role);

// Assign a concrete scancode to the given layout and key role.
void Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode);

// Retrieve the assigned scancode for the given layout and key role.
INPUT_SCANCODE Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role);

// If there is anything pressed on the keyboard, assigns the pressed key to the
// given key role and returns true. If nothing is pressed, immediately returns
// false.
bool Input_ReadAndAssignKey(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Given the input layout and input key role, get the assigned key name.
const char *Input_GetKeyName(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Checks if the given key is being pressed.
bool Input_CheckKeypress(const char *check_key);

// Reset a given layout to the default.
void Input_ResetLayout(INPUT_LAYOUT layout_num);

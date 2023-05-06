#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;

void Input_Init(void);
void Input_Shutdown(void);
void Input_Update(void);

// Checks the current keyboard layout for key conflicts.
void Input_CheckConflicts(CONTROL_MODE mode, INPUT_LAYOUT layout_num);

// Checks the current controller layout for button conflicts.
void Input_CheckControllerConflicts(INPUT_LAYOUT layout_num);

// Returns whether the key assigned to the given role is also used elsewhere
// within the custom layout.
bool Input_IsKeyConflicted(CONTROL_MODE mode, INPUT_ROLE role);

// Assign a concrete scancode to the given layout and key role.
void Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode);

// Assign a concrete button to the given layout and key role.
void Input_AssignButton(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, int16_t button);

// Assign a concrete axis to the given layout and key role.
void Input_AssignAxis(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, int16_t axis, int16_t axis_dir);

// Retrieve the assigned scancode for the given layout and key role.
INPUT_SCANCODE Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Get a unique button or axis for the given layout and key role. Offets with
// axis direction.
int16_t Input_GetUniqueBind(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Get the assigned button or axis type for the given layout and key role.
int16_t Input_GetAssignedButtonType(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Get the assigned controller button or axis for the given layout and key role.
int16_t Input_GetAssignedBind(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Get the assigned axis direction for the given layout and key role.
int16_t Input_GetAssignedAxisDir(INPUT_LAYOUT layout_num, INPUT_ROLE role);

// If there is anything pressed on the keyboard, assigns the pressed key to the
// given key role and returns true. If nothing is pressed, immediately returns
// false.
bool Input_ReadAndAssignKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Given the input layout and input key role, get the assigned key name.
const char *Input_GetKeyName(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role);

// Checks if the given key is being pressed.
bool Input_CheckKeypress(const char *check_key);

// Gets the name of a controller button from the string name depending on the
// controller type.
const char *Input_GetButtonName(
    INPUT_LAYOUT layout_num, const char *button_name);

// Checks if the given button is being pressed.
bool Input_CheckButtonPress(const char *button_name);

// Reset a given layout to the default.
void Input_ResetLayout(CONTROL_MODE mode, INPUT_LAYOUT layout_num);

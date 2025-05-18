#pragma once

#include "../../json.h"

#include <stdint.h>

typedef enum {
#undef INPUT_ROLE_DEFINE
#define INPUT_ROLE_DEFINE(role_name, state_name) INPUT_ROLE_##role_name,
#include "roles.def"
    INPUT_ROLE_NUMBER_OF,
} INPUT_ROLE;

typedef union {
    uint64_t any;
    struct {
#undef INPUT_ROLE_DEFINE
#define INPUT_ROLE_DEFINE(role_name, state_name) uint64_t state_name : 1;
#include "roles.def"
    };
} INPUT_STATE;

typedef enum {
    INPUT_BACKEND_KEYBOARD,
    INPUT_BACKEND_CONTROLLER,
    INPUT_BACKEND_NUMBER_OF,
} INPUT_BACKEND;

typedef enum {
    INPUT_LAYOUT_DEFAULT,
    INPUT_LAYOUT_CUSTOM_1,
    INPUT_LAYOUT_CUSTOM_2,
    INPUT_LAYOUT_CUSTOM_3,
    INPUT_LAYOUT_NUMBER_OF,
} INPUT_LAYOUT;

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;

void Input_Init(void);
void Input_Shutdown(void);
void Input_Discover(void);
void Input_Update(void);

// Checks whether the given role can be assigned to by the player.
// Hard-coded roles are exempt from conflict checks (eg will never flash in the
// controls dialog).
bool Input_IsRoleRebindable(INPUT_ROLE role);

// Returns whether the key assigned to the given role is also used elsewhere
// within the custom layout.
bool Input_IsKeyConflicted(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Given the input layout and input key role, check if the assorted key is
// pressed, bypassing Input_Update.
bool Input_IsPressed(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// If there is anything pressed, assigns the pressed key to the given key role
// and returns true. If nothing is pressed, immediately returns false.
bool Input_ReadAndAssignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Remove assigned key from a given key role.
void Input_UnassignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Get the layout human-readable name.
const char *Input_GetLayoutName(const INPUT_LAYOUT layout);

// Given the input layout and input key role, get the assigned key name.
const char *Input_GetKeyName(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Reset a given layout to the default.
void Input_ResetLayout(INPUT_BACKEND backend, INPUT_LAYOUT layout);

// Disables updating g_Input.
void Input_EnterListenMode(void);

// Enables updating g_Input.
void Input_ExitListenMode(void);

// Checks whether updates are disabled.
bool Input_IsInListenMode(void);

// Restores the user configuration by converting the JSON object back into the
// original input layout.
bool Input_AssignFromJSONObject(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, JSON_OBJECT *bind_obj);

// Converts the original input layout into a JSON object for storing the user
// configuration.
bool Input_AssignToJSONObject(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, JSON_OBJECT *bind_obj,
    INPUT_ROLE role);

INPUT_STATE Input_GetDebounced(const INPUT_STATE input);

extern const char *Input_GetRoleName(INPUT_ROLE role);

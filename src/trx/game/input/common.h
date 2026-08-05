#pragma once

#include <trx/config/enum.h>
#include <trx/core/json.h>
#include <trx/game/input/enum.h>

#include <stdint.h>

#define INPUT_COMBO_MAX_KEYS 3
#define INPUT_BINDING_SLOTS 2

#define INPUT_STATE_ANY_WORDS ((INPUT_ROLE_NUMBER_OF + 63) / 64)

typedef union {
    uint64_t any[INPUT_STATE_ANY_WORDS];
    struct {
#define X_INPUT_ROLE(role_name, state_name) uint64_t state_name : 1;
#include <trx/game/input/roles.def>
#undef X_INPUT_ROLE
    };
} INPUT_STATE;

extern INPUT_STATE g_Input;
extern INPUT_STATE g_InputDB;
extern INPUT_STATE g_OldInputDB;

void Input_Init(void);
void Input_Shutdown(void);
void Input_Update(void);

// Reconciles the connected devices with the current configuration: enabled
// backends acquire the hardware that is present, disabled ones release it.
void Input_Discover(void);
void Input_Reset(void);

// Checks whether the given backend is available. A disabled backend is not
// polled, not offered in the controls dialog, and holds no OS resources.
bool Input_IsBackendEnabled(INPUT_BACKEND backend);

// Checks whether the given role can be assigned to by the player.
// Hard-coded roles are exempt from conflict checks (eg will never flash in the
// controls dialog).
bool Input_IsRoleRebindable(INPUT_ROLE role);

// Checks whether the given role can be completely unbound by the player.
bool Input_IsRoleUnbindable(INPUT_ROLE role);

// Checks whether the given role uses keys that fire immediately and cannot
// be the first key of a combo (movement, jump, action, roll).
bool Input_IsRoleImmediate(INPUT_ROLE role);

// Checks whether the given role is a held state that fires immediately but
// can still be the first key of a combo (look, walk). Unlike immediate
// roles, these don't block combo formation because the held state is not
// disrupted when a longer combo completes.
bool Input_IsRoleSustained(INPUT_ROLE role);

// Checks whether the given role captures other inputs when held, redirecting
// them to different actions (e.g. look mode repurposes movement keys for
// camera control). Capturing roles can start combos with immediate keys
// because the immediate keys lose their normal meaning while captured.
bool Input_IsRoleCapturing(INPUT_ROLE role);

// Returns whether the key assigned to the given role is also used elsewhere
// within the custom layout.
bool Input_IsKeyConflicted(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// Checks if the key is currently pressed. Tied to Input_Update(), so updates
// at most at the game running FPS.
bool Input_IsPressed(INPUT_ROLE role);

// Checks if the key is currently pressed with a debounce, e.g. only true
// for the game frame the player starts to hold the key at.
bool Input_IsPressedDB(INPUT_ROLE role);

// Given the input layout and input key role, check if the assorted key is
// pressed, bypassing Input_Update.
bool Input_IsPressedEx(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role);

// If there is anything pressed, assigns the pressed key to the given key role
// and returns true. If nothing is pressed, immediately returns false.
bool Input_ReadAndAssignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);

// Remove assigned key from a given key role.
void Input_UnassignRole(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);

// Get a stable pointer to the layout human-readable name.
const char *const *Input_GetLayoutNamePtr(const INPUT_LAYOUT layout);

// Given the input layout and input key role, get the assigned key name.
const char *Input_GetKeyName(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, INPUT_ROLE role, int32_t slot);

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
    INPUT_BACKEND backend, INPUT_LAYOUT layout, const JSON_OBJECT *bind_obj);

// Converts the original input layout into a JSON object for storing the user
// configuration.
bool Input_AssignToJSONObject(
    INPUT_BACKEND backend, INPUT_LAYOUT layout, JSON_OBJECT *bind_obj,
    INPUT_ROLE role, int32_t slot);

// Return a copy of the input state with only newly pressed roles set.
INPUT_STATE Input_GetDebounced(const INPUT_STATE input);

// Get the human-readable name of the given role.
const char *Input_GetRoleName(INPUT_ROLE role);

// Reset all roles in the input state to inactive.
void InputState_Clear(INPUT_STATE *state);

// Copy the source input state into the destination.
void InputState_Copy(INPUT_STATE *dst, INPUT_STATE src);

// Check whether any role is active in the input state.
bool InputState_IsAnyPressed(INPUT_STATE state);

// Check whether the given role is active in the input state.
bool InputState_GetRole(INPUT_STATE state, INPUT_ROLE role);

// Set or clear the given role in the input state.
void InputState_SetRole(INPUT_STATE *state, INPUT_ROLE role, bool value);

// Clear the given role in the input state.
void InputState_ClearRole(INPUT_STATE *state, INPUT_ROLE role);

// The input system, reduced to what a surface test needs: every role is bound
// to one key on the keyboard, nothing is pressed, and rebinding does nothing.
// The real one reads devices and keeps the player's layouts.

#include <trx/game/input.h>

INPUT_STATE g_Input = {};
INPUT_STATE g_InputDB = {};

const char *Input_GetKeyName(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return slot == 0 ? "K" : nullptr;
}

const char *Input_GetRoleName(const INPUT_ROLE role)
{
    return "Jump";
}

const char *const *Input_GetLayoutNamePtr(const INPUT_LAYOUT layout)
{
    static const char *names[INPUT_LAYOUT_NUMBER_OF] = {
        "Default",
        "Custom 1",
        "Custom 2",
        "Custom 3",
    };
    return &names[layout];
}

bool Input_IsBackendEnabled(const INPUT_BACKEND backend)
{
    return backend == INPUT_BACKEND_KEYBOARD;
}

bool Input_IsRoleRebindable(const INPUT_ROLE role)
{
    return true;
}

bool Input_IsRoleUnbindable(const INPUT_ROLE role)
{
    return true;
}

bool Input_IsKeyConflicted(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role)
{
    return false;
}

bool Input_IsHeld(const INPUT_ROLE role)
{
    return false;
}

bool Input_IsPressed(const INPUT_ROLE role)
{
    return false;
}

void Input_HoldOffRole(const INPUT_ROLE role)
{
}

bool Input_ReadAndAssignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
    return false;
}

void Input_UnassignRole(
    const INPUT_BACKEND backend, const INPUT_LAYOUT layout,
    const INPUT_ROLE role, const int32_t slot)
{
}

void Input_ResetLayout(const INPUT_BACKEND backend, const INPUT_LAYOUT layout)
{
}

void Input_EnterListenMode(void)
{
}

void Input_ExitListenMode(void)
{
}

bool Input_IsInListenMode(void)
{
    return false;
}

bool InputState_IsAnyPressed(const INPUT_STATE state)
{
    return false;
}

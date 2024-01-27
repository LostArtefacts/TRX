#include "game/input.h"

#include "config.h"
#include "global/vars.h"
#include "specific/s_input.h"

#include <stdint.h>

#define DELAY_FRAMES 12
#define HOLD_FRAMES 3

INPUT_STATE g_Input = { 0 };
INPUT_STATE g_InputDB = { 0 };
INPUT_STATE g_OldInputDB = { 0 };

static bool m_KeyConflict[INPUT_ROLE_NUMBER_OF] = { false };
static bool m_BtnConflict[INPUT_ROLE_NUMBER_OF] = { false };
static int32_t m_HoldBack = 0;
static int32_t m_HoldForward = 0;
static int32_t m_HoldLook = 0;

static void Input_CheckChangeTarget(INPUT_STATE *input);
static INPUT_STATE Input_GetDebounced(INPUT_STATE input);

static void Input_CheckChangeTarget(INPUT_STATE *input)
{
    if (g_Lara.gun_status != LGS_READY) {
        return;
    }

    if (input->look) {
        if (m_HoldLook >= 6)
            m_HoldLook = 100;
        else {
            input->look = 0;
            m_HoldLook++;
        }
    } else {
        if (m_HoldLook && m_HoldLook != 100) {
            input->change_target = 1;
        }
        m_HoldLook = 0;
    }
}

static INPUT_STATE Input_GetDebounced(INPUT_STATE input)
{
    INPUT_STATE result;
    result.any = input.any & ~g_OldInputDB.any;

    // Allow holding down key to move faster
    if (input.forward || !input.back) {
        m_HoldBack = 0;
    } else if (input.back && ++m_HoldBack >= DELAY_FRAMES + HOLD_FRAMES) {
        result.back = 1;
        result.menu_down = 1;
        m_HoldBack = DELAY_FRAMES;
    }

    if (!input.forward || input.back) {
        m_HoldForward = 0;
    } else if (input.forward && ++m_HoldForward >= DELAY_FRAMES + HOLD_FRAMES) {
        result.forward = 1;
        result.menu_up = 1;
        m_HoldForward = DELAY_FRAMES;
    }

    g_OldInputDB = input;
    return result;
}

void Input_CheckConflicts(CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    if (mode == CM_KEYBOARD) {
        for (INPUT_ROLE role1 = 0; role1 < INPUT_ROLE_NUMBER_OF; role1++) {
            INPUT_SCANCODE scancode1 =
                Input_GetAssignedScancode(layout_num, role1);
            m_KeyConflict[role1] = false;

            for (INPUT_ROLE role2 = 0; role2 < INPUT_ROLE_NUMBER_OF; role2++) {
                if (role1 == role2) {
                    continue;
                }
                INPUT_SCANCODE scancode2 =
                    Input_GetAssignedScancode(layout_num, role2);
                if (scancode1 == scancode2) {
                    m_KeyConflict[role1] = true;
                }
            }
        }
    } else {
        for (INPUT_ROLE role1 = 0; role1 < INPUT_ROLE_NUMBER_OF; role1++) {
            int16_t bind1 = Input_GetUniqueBind(layout_num, role1);
            m_BtnConflict[role1] = false;

            for (INPUT_ROLE role2 = 0; role2 < INPUT_ROLE_NUMBER_OF; role2++) {
                if (role1 == role2) {
                    continue;
                }
                int16_t bind2 = Input_GetUniqueBind(layout_num, role2);
                if (bind1 == bind2) {
                    m_BtnConflict[role1] = true;
                }
            }
        }
    }
}

void Input_Init(void)
{
    S_Input_Init();
}

void Input_Shutdown(void)
{
    S_Input_Shutdown();
}

void Input_InitController(void)
{
    S_Input_InitController();
}

void Input_ShutdownController(void)
{
    S_Input_ShutdownController();
}

void Input_Update(void)
{
    g_Input = S_Input_GetCurrentState(
        g_Config.input.layout, g_Config.input.cntlr_layout);

    g_Input.menu_up |= g_Input.forward;
    g_Input.menu_down |= g_Input.back;
    g_Input.menu_left |= g_Input.left;
    g_Input.menu_right |= g_Input.right;
    g_Input.menu_back |= g_Input.option;
    g_Input.option &= g_Camera.type != CAM_CINEMATIC;
    g_Input.roll |= g_Input.forward && g_Input.back;
    if (g_Input.left && g_Input.right) {
        g_Input.left = 0;
        g_Input.right = 0;
    }

    if (!g_Config.enable_cheats) {
        g_Input.item_cheat = 0;
        g_Input.fly_cheat = 0;
        g_Input.level_skip_cheat = 0;
        g_Input.turbo_cheat = 0;
        g_Input.health_cheat = 0;
    }

    if (g_Config.enable_tr3_sidesteps) {
        if (g_Input.slow && !g_Input.forward && !g_Input.back
            && !g_Input.step_left && !g_Input.step_right) {
            if (g_Input.left) {
                g_Input.left = 0;
                g_Input.step_left = 1;
            } else if (g_Input.right) {
                g_Input.right = 0;
                g_Input.step_right = 1;
            }
        }
    }

    if (g_Config.enable_target_change) {
        Input_CheckChangeTarget(&g_Input);
    }

    g_InputDB = Input_GetDebounced(g_Input);
}

bool Input_IsKeyConflicted(CONTROL_MODE mode, INPUT_ROLE role)
{
    if (mode == CM_KEYBOARD) {
        return m_KeyConflict[role];
    } else {
        return m_BtnConflict[role];
    }
}

INPUT_SCANCODE Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetAssignedScancode(layout_num, role);
}

int16_t Input_GetUniqueBind(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetUniqueBind(layout_num, role);
}

int16_t Input_GetAssignedButtonType(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetAssignedButtonType(layout_num, role);
}

int16_t Input_GetAssignedBind(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetAssignedBind(layout_num, role);
}

int16_t Input_GetAssignedAxisDir(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetAssignedAxisDir(layout_num, role);
}

void Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode)
{
    S_Input_AssignScancode(layout_num, role, scancode);
    Input_CheckConflicts(CM_KEYBOARD, layout_num);
}

void Input_AssignButton(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, int16_t button)
{
    S_Input_AssignButton(layout_num, role, button);
    Input_CheckConflicts(CM_CONTROLLER, layout_num);
}

void Input_AssignAxis(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, int16_t axis, int16_t axis_dir)
{
    S_Input_AssignAxis(layout_num, role, axis, axis_dir);
    Input_CheckConflicts(CM_CONTROLLER, layout_num);
}

bool Input_ReadAndAssignKey(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    if (S_Input_ReadAndAssignKey(mode, layout_num, role)) {
        Input_CheckConflicts(mode, layout_num);
        return true;
    }
    return false;
}

const char *Input_GetKeyName(
    CONTROL_MODE mode, INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetKeyName(mode, layout_num, role);
}

bool Input_CheckKeypress(const char *check_key)
{
    return S_Input_CheckKeypress(check_key);
}

const char *Input_GetButtonName(
    INPUT_LAYOUT layout_num, const char *button_name)
{
    return S_Input_GetButtonNameFromString(layout_num, button_name);
}

bool Input_CheckButtonPress(const char *button_name)
{
    return S_Input_CheckButtonPress(button_name);
}

void Input_ResetLayout(CONTROL_MODE mode, INPUT_LAYOUT layout_num)
{
    if (mode == CM_KEYBOARD) {
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            INPUT_SCANCODE scancode =
                Input_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role);
            Input_AssignScancode(layout_num, role, scancode);
        }
    } else {
        S_Input_ResetControllerToDefault(layout_num);
    }
}

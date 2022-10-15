#include "game/input.h"

#include "config.h"
#include "game/clock.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "global/vars.h"
#include "specific/s_input.h"

#include <stdint.h>

#define DELAY_FRAMES 12
#define HOLD_FRAMES 3

INPUT_STATE g_Input = { 0 };
INPUT_STATE g_InputDB = { 0 };
INPUT_STATE g_OldInputDB = { 0 };

static bool m_KeyConflictWithUser[INPUT_ROLE_NUMBER_OF] = { false };
static bool m_KeyConflictWithDefault[INPUT_ROLE_NUMBER_OF] = { false };
static int32_t m_HoldBack = 0;
static int32_t m_HoldForward = 0;

static void Input_CheckConflicts(INPUT_LAYOUT layout_num);
static INPUT_STATE Input_GetDebounced(INPUT_STATE input);

static void Input_CheckConflicts(INPUT_LAYOUT layout_num)
{
    for (INPUT_ROLE role1 = 0; role1 < INPUT_ROLE_NUMBER_OF; role1++) {
        INPUT_SCANCODE scancode1_default =
            Input_GetAssignedScancode(INPUT_LAYOUT_DEFAULT, role1);
        m_KeyConflictWithDefault[role1] = false;

        INPUT_SCANCODE scancode1_user =
            Input_GetAssignedScancode(layout_num, role1);
        m_KeyConflictWithUser[role1] = false;

        for (INPUT_ROLE role2 = 0; role2 < INPUT_ROLE_NUMBER_OF; role2++) {
            if (role1 == role2) {
                continue;
            }

            INPUT_SCANCODE scancode2_user =
                Input_GetAssignedScancode(layout_num, role2);

            if (scancode1_user == scancode2_user) {
                m_KeyConflictWithUser[role1] = true;
            }
            if (scancode1_default == scancode2_user) {
                m_KeyConflictWithDefault[role1] = true;
            }
        }
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
        m_HoldBack = DELAY_FRAMES;
    }

    if (!input.forward || input.back) {
        m_HoldForward = 0;
    } else if (input.forward && ++m_HoldForward >= DELAY_FRAMES + HOLD_FRAMES) {
        result.forward = 1;
        m_HoldForward = DELAY_FRAMES;
    }

    g_OldInputDB = input;
    return result;
}

void Input_Init(void)
{
    S_Input_Init();
}

void Input_Shutdown(void)
{
    S_Input_Shutdown();
}

void Input_Update(void)
{
    g_Input = S_Input_GetCurrentState(g_Config.input.layout);

    g_Input.select |= g_Input.action;
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

    g_InputDB = Input_GetDebounced(g_Input);

    if (g_Config.enable_numeric_keys) {
        if (g_InputDB.equip_pistols && Inv_RequestItem(O_GUN_ITEM)) {
            g_Lara.request_gun_type = LGT_PISTOLS;
        } else if (g_InputDB.equip_shotgun && Inv_RequestItem(O_SHOTGUN_ITEM)) {
            g_Lara.request_gun_type = LGT_SHOTGUN;
        } else if (g_InputDB.equip_magnums && Inv_RequestItem(O_MAGNUM_ITEM)) {
            g_Lara.request_gun_type = LGT_MAGNUMS;
        } else if (g_InputDB.equip_uzis && Inv_RequestItem(O_UZI_ITEM)) {
            g_Lara.request_gun_type = LGT_UZIS;
        }
    }

    if (g_InputDB.use_small_medi && Inv_RequestItem(O_MEDI_OPTION)) {
        Lara_UseItem(O_MEDI_OPTION);
    } else if (g_InputDB.use_big_medi && Inv_RequestItem(O_BIGMEDI_OPTION)) {
        Lara_UseItem(O_BIGMEDI_OPTION);
    }

    if (g_InputDB.toggle_bilinear_filter) {
        g_Config.rendering.enable_bilinear_filter ^= true;
    }

    if (g_InputDB.toggle_perspective_filter) {
        g_Config.rendering.enable_perspective_filter ^= true;
    }

    if (g_InputDB.toggle_fps_counter) {
        g_Config.rendering.enable_fps_counter ^= true;
    }

    if (g_InputDB.turbo_cheat) {
        Clock_CycleTurboSpeed();
    }
}

bool Input_IsKeyConflictedWithUser(INPUT_ROLE role)
{
    return m_KeyConflictWithUser[role];
}

bool Input_IsKeyConflictedWithDefault(INPUT_ROLE role)
{
    return m_KeyConflictWithDefault[role];
}

INPUT_SCANCODE Input_GetAssignedScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetAssignedScancode(layout_num, role);
}

void Input_AssignScancode(
    INPUT_LAYOUT layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode)
{
    S_Input_AssignScancode(layout_num, role, scancode);
    Input_CheckConflicts(layout_num);
}

bool Input_ReadAndAssignKey(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    if (S_Input_ReadAndAssignKey(layout_num, role)) {
        Input_CheckConflicts(layout_num);
        return true;
    }
    return false;
}

const char *Input_GetKeyName(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetKeyName(layout_num, role);
}

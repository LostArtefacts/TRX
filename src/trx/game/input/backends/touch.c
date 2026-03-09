#include <trx/game/input/backends/touch.h>

#include <trx/game/input/common.h>

#include <emscripten.h>
#include <stdint.h>

static bool m_State[INPUT_ROLE_NUMBER_OF] = {};

EMSCRIPTEN_KEEPALIVE
void Touch_SetState(const int32_t role, const int32_t pressed)
{
    if (role >= 0 && role < INPUT_ROLE_NUMBER_OF) {
        m_State[role] = pressed != 0;
    }
}

// clang-format off
EM_JS(void, js_export_input_roles, (
    int up, int down, int left, int right,
    int slow, int jump, int action, int draw_weapon,
    int look, int roll, int inventory, int pause), {
    Module.INPUT_ROLE = {
        UP: up, DOWN: down, LEFT: left, RIGHT: right,
        SLOW: slow, JUMP: jump, ACTION: action,
        DRAW_WEAPON: draw_weapon, LOOK: look,
        ROLL: roll, INVENTORY: inventory, PAUSE: pause,
    };
})
// clang-format on

static void M_Init(void)
{
    js_export_input_roles(
        INPUT_ROLE_UP, INPUT_ROLE_DOWN, INPUT_ROLE_LEFT, INPUT_ROLE_RIGHT,
        INPUT_ROLE_SLOW, INPUT_ROLE_JUMP, INPUT_ROLE_ACTION,
        INPUT_ROLE_DRAW_WEAPON, INPUT_ROLE_LOOK, INPUT_ROLE_ROLL,
        INPUT_ROLE_INVENTORY, INPUT_ROLE_PAUSE);
}

static bool M_IsPressed(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    (void)layout;
    if (role >= 0 && role < INPUT_ROLE_NUMBER_OF) {
        return m_State[role];
    }
    return false;
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    (void)layout;
    result->menu_confirm |= result->action;
    result->menu_back |= result->jump;
    result->menu_skip = result->menu_confirm || result->menu_back;
    return true;
}

INPUT_BACKEND_IMPL g_Input_Touch = {
    .init = M_Init,
    .shutdown = nullptr,
    .discover = nullptr,
    .custom_update = M_CustomUpdate,
    .process_event = nullptr,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = nullptr,
    .get_name = nullptr,
    .unassign_role = nullptr,
    .assign_from_json_object = nullptr,
    .assign_to_json_object = nullptr,
    .reset_layout = nullptr,
    .read_and_assign = nullptr,
};

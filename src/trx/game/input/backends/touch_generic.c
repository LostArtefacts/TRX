#include <trx/game/input/backends/touch.h>

static bool M_IsPressed(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    (void)layout;
    (void)role;
    return false;
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    (void)result;
    (void)layout;
    return false;
}

INPUT_BACKEND_IMPL g_Input_Touch = {
    .init = nullptr,
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

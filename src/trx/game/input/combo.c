#include <trx/game/input/combo.h>

#include <trx/game/input/common.h>

static bool M_IsKeyNonCapturingSustained(
    const INPUT_LAYOUT layout, const void *const key,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (!Input_IsRoleSustained(r) || Input_IsRoleCapturing(r)) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, r, s);
            if (b.key_count == 1 && keys_equal(Input_ComboKeyAt(&b, 0), key)) {
                return true;
            }
        }
    }
    return false;
}

bool Input_ComboIsProperSubset(
    const INPUT_COMBO_BINDING sub, const INPUT_COMBO_BINDING super,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    if (sub.key_count == 0 || sub.key_count >= super.key_count) {
        return false;
    }
    for (int32_t i = 0; i < sub.key_count; i++) {
        bool found = false;
        for (int32_t j = 0; j < super.key_count; j++) {
            if (keys_equal(
                    Input_ComboKeyAt(&sub, i), Input_ComboKeyAt(&super, j))) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool Input_ComboHasLonger(
    const INPUT_LAYOUT layout, const INPUT_ROLE skip_role,
    const INPUT_COMBO_BINDING bind, const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (r == skip_role) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, r, s);
            if (Input_ComboIsProperSubset(bind, b, keys_equal)
                && keys_equal(
                    Input_ComboKeyAt(&bind, 0), Input_ComboKeyAt(&b, 0))) {
                return true;
            }
        }
    }
    return false;
}

bool Input_ComboIsStarter(
    const INPUT_LAYOUT layout, const void *const key,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, r, s);
            if (b.key_count >= 2 && keys_equal(Input_ComboKeyAt(&b, 0), key)) {
                return true;
            }
        }
    }
    return false;
}

INPUT_ROLE Input_ComboFindDeferrableRole(
    const INPUT_LAYOUT layout, const void *const key,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (Input_IsRoleImmediate(r) || Input_IsRoleSustained(r)
            || !Input_IsRoleRebindable(r)) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, r, s);
            if (b.key_count == 1 && keys_equal(Input_ComboKeyAt(&b, 0), key)) {
                return r;
            }
        }
    }
    return (INPUT_ROLE)-1;
}

bool Input_ComboIsKeyImmediate(
    const INPUT_LAYOUT layout, const void *const key,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (!Input_IsRoleImmediate(r) || !Input_IsRoleRebindable(r)) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, r, s);
            if (b.key_count == 1 && keys_equal(Input_ComboKeyAt(&b, 0), key)) {
                return true;
            }
        }
    }
    return false;
}

bool Input_ComboStartsWithImmediate(
    const INPUT_LAYOUT layout, const INPUT_COMBO_BINDING bind,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    if (bind.key_count < 2) {
        return false;
    }
    return Input_ComboIsKeyImmediate(
        layout, Input_ComboKeyAt(&bind, 0), get_binding, keys_equal);
}

bool Input_ComboSustainedHasImmediate(
    const INPUT_LAYOUT layout, const INPUT_COMBO_BINDING bind,
    const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal)
{
    if (bind.key_count < 2) {
        return false;
    }
    if (!M_IsKeyNonCapturingSustained(
            layout, Input_ComboKeyAt(&bind, 0), get_binding, keys_equal)) {
        return false;
    }
    for (int32_t k = 1; k < bind.key_count; k++) {
        if (Input_ComboIsKeyImmediate(
                layout, Input_ComboKeyAt(&bind, k), get_binding, keys_equal)) {
            return true;
        }
    }
    return false;
}

void Input_ComboCheckConflicts(
    const INPUT_LAYOUT layout, const INPUT_COMBO_GET_BINDING get_binding,
    const INPUT_COMBO_KEYS_EQUAL keys_equal,
    bool conflicts[INPUT_ROLE_NUMBER_OF])
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (conflicts[role]) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const INPUT_COMBO_BINDING b = get_binding(layout, role, s);
            if (Input_ComboStartsWithImmediate(
                    layout, b, get_binding, keys_equal)
                || Input_ComboSustainedHasImmediate(
                    layout, b, get_binding, keys_equal)) {
                conflicts[role] = true;
                break;
            }
        }
    }
}

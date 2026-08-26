#include <trx/game/input/backends/touch.h>

#include <trx/config.h>
#include <trx/config/common.h>
#include <trx/core/utils.h>
#include <trx/game/input/backends/internal.h>
#include <trx/game/input/combo.h>
#include <trx/game/input/common.h>
#include <trx/game/input/names.h>
#include <trx/game/ui/touch_overlay.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_touch.h>
#include <string.h>

#define M_MAX_POSITIONS 64

typedef struct {
    int32_t pos_count;
    int32_t positions[INPUT_COMBO_MAX_KEYS];
} M_BINDING;

typedef struct {
    M_BINDING slots[INPUT_BINDING_SLOTS];
} M_ROLE_BINDING;

// Position names as glyph escape strings (matching text_autogen.def).
// clang-format off
static const char *const m_PosGlyphs[] = {
    "\\{controller lstick up}",    // TOUCH_POS_DPAD_UP
    "\\{controller lstick down}",  // TOUCH_POS_DPAD_DOWN
    "\\{controller lstick left}",  // TOUCH_POS_DPAD_LEFT
    "\\{controller lstick right}", // TOUCH_POS_DPAD_RIGHT
    "\\{touch jump}",          // m_ButtonDefs[1]
    "\\{touch action}",        // m_ButtonDefs[2]
    "\\{touch walk}",          // m_ButtonDefs[3]
    "\\{touch look}",          // m_ButtonDefs[4]
    "\\{touch roll}",          // m_ButtonDefs[5]
    "\\{touch draw weapon}",   // m_ButtonDefs[6]  (TR1/2)
    "\\{touch run}",           // m_ButtonDefs[7]  (TR3 sprint)
    "\\{touch crouch}",        // m_ButtonDefs[8]  (TR3)
    "\\{touch draw weapon}",   // m_ButtonDefs[9]  (TR3 draw weapon)
    "\\{touch menu}",          // m_ButtonDefs[10]
    "\\{touch pause}",         // m_ButtonDefs[11]
};
// clang-format on

static M_ROLE_BINDING m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF];
static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF];

// Per-position press state (set by overlay during gameplay).
static bool m_PosState[M_MAX_POSITIONS];

// Combo capture buffer for listen mode (filled during M_ReadAndAssign).
static M_BINDING m_CaptureBuffer = {};
static bool m_CaptureActive = false;

// Per-position tracking for combo prefix deferral.
static bool m_PrefixWasHeld[M_MAX_POSITIONS];
static bool m_PrefixComboFired[M_MAX_POSITIONS];

// Per-position press tick: records the tick each position most recently
// transitioned from released to held. Used to detect combos that "start"
// on an immediate role the user was already holding.
static uint32_t m_PosDownTick[M_MAX_POSITIONS];
static uint32_t m_Tick;

// Per-role deferral tracking for combo disambiguation.
static bool m_RoleWasActive[INPUT_ROLE_NUMBER_OF];
static bool m_RoleLongerFired[INPUT_ROLE_NUMBER_OF];

// --- Helpers ---

static const M_BINDING *M_GetBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    return &m_Layout[layout][role].slots[slot];
}

static bool M_BindingsEqual(const M_BINDING *const a, const M_BINDING *const b)
{
    if (a->pos_count != b->pos_count) {
        return false;
    }
    for (int32_t i = 0; i < a->pos_count; i++) {
        if (a->positions[i] != b->positions[i]) {
            return false;
        }
    }
    return true;
}

static bool M_CheckConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role1, const INPUT_ROLE role2)
{
    for (int32_t s1 = 0; s1 < INPUT_BINDING_SLOTS; s1++) {
        const M_BINDING *b1 = M_GetBinding(layout, role1, s1);
        if (b1->pos_count == 0) {
            continue;
        }
        for (int32_t s2 = 0; s2 < INPUT_BINDING_SLOTS; s2++) {
            const M_BINDING *b2 = M_GetBinding(layout, role2, s2);
            if (b2->pos_count == 0) {
                continue;
            }
            if (M_BindingsEqual(b1, b2)) {
                return true;
            }
        }
    }
    return false;
}

static void M_AssignConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const bool conflict)
{
    m_Conflicts[layout][role] = conflict;
}

static bool M_CheckBinding(const M_BINDING *const bind)
{
    if (bind->pos_count == 0) {
        return false;
    }
    for (int32_t k = 0; k < bind->pos_count; k++) {
        if (!m_PosState[bind->positions[k]]) {
            return false;
        }
    }
    return true;
}

static bool M_IsProperSubset(
    const M_BINDING *const sub, const M_BINDING *const super)
{
    if (sub->pos_count == 0 || sub->pos_count >= super->pos_count) {
        return false;
    }
    for (int32_t i = 0; i < sub->pos_count; i++) {
        bool found = false;
        for (int32_t j = 0; j < super->pos_count; j++) {
            if (sub->positions[i] == super->positions[j]) {
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

static const M_BINDING *M_GetPressedBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
        const M_BINDING *bind = M_GetBinding(layout, role, slot);
        if (M_CheckBinding(bind)) {
            return bind;
        }
    }
    return nullptr;
}

static bool M_HasLongerCombo(
    const INPUT_LAYOUT layout, const INPUT_ROLE skip_role,
    const M_BINDING *const bind)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (r == skip_role) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, r, s);
            if (M_IsProperSubset(bind, b)) {
                return true;
            }
        }
    }
    return false;
}

static bool M_IsComboPosition(const INPUT_LAYOUT layout, const int32_t pos)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, r, s);
            if (b->pos_count < 2) {
                continue;
            }
            for (int32_t k = 0; k < b->pos_count; k++) {
                if (b->positions[k] == pos) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool M_IsPositionImmediate(const INPUT_LAYOUT layout, const int32_t pos)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (!Input_IsRoleImmediate(r)) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, r, s);
            for (int32_t k = 0; k < b->pos_count; k++) {
                if (b->positions[k] == pos) {
                    return true;
                }
            }
        }
    }
    return false;
}

static INPUT_ROLE M_FindSinglePosRole(
    const INPUT_LAYOUT layout, const int32_t pos)
{
    for (INPUT_ROLE r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        if (Input_IsRoleImmediate(r)) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, r, s);
            if (b->pos_count == 1 && b->positions[0] == pos) {
                return r;
            }
        }
    }
    return (INPUT_ROLE)-1;
}

static bool M_ComboStartsWithImmediate(
    const INPUT_LAYOUT layout, const M_BINDING *const bind)
{
    if (bind->pos_count < 2) {
        return false;
    }
    return M_IsPositionImmediate(layout, bind->positions[0]);
}

static void M_CheckConflicts(const INPUT_LAYOUT layout)
{
    Input_ConflictHelper(layout, M_CheckConflict, M_AssignConflict);
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (m_Conflicts[layout][role]) {
            continue;
        }
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, role, s);
            if (M_ComboStartsWithImmediate(layout, b)) {
                m_Conflicts[layout][role] = true;
                break;
            }
        }
    }
}

static void M_AssignBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const M_BINDING *const bind)
{
    m_Layout[layout][role].slots[slot] = *bind;
    M_CheckConflicts(layout);
}

static bool M_CaptureHasPos(const int32_t pos)
{
    for (int32_t i = 0; i < m_CaptureBuffer.pos_count; i++) {
        if (m_CaptureBuffer.positions[i] == pos) {
            return true;
        }
    }
    return false;
}

// --- Combo adapter functions for the shared combo layer ---

static INPUT_COMBO_BINDING M_GetComboBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const M_BINDING *const b = M_GetBinding(layout, role, slot);
    return (INPUT_COMBO_BINDING) {
        .key_count = b->pos_count,
        .keys = b->positions,
        .key_stride = sizeof(b->positions[0]),
    };
}

static INPUT_COMBO_BINDING M_ToCombo(const M_BINDING *const b)
{
    return (INPUT_COMBO_BINDING) {
        .key_count = b->pos_count,
        .keys = b->positions,
        .key_stride = sizeof(b->positions[0]),
    };
}

static bool M_ComboKeysEqual(const void *const a, const void *const b)
{
    return *(const int32_t *)a == *(const int32_t *)b;
}

static uint32_t M_GetPressTick(const void *const key)
{
    const int32_t pos = *(const int32_t *)key;
    if (pos < 0 || pos >= M_MAX_POSITIONS) {
        return 0;
    }
    return m_PosDownTick[pos];
}

// --- Backend interface ---

static void M_InitDefaultBindings(void)
{
    const int32_t num_pos = TouchOverlay_GetPositionCount();
    // Clear all
    memset(m_Layout, 0, sizeof(m_Layout));

    // Populate the default layout from every position the overlay knows
    // about. Positions whose features are currently disabled will just be
    // unreachable (the overlay hides their buttons), which avoids baking the
    // current game's feature set into the config on first run.
    for (int32_t p = 0; p < num_pos; p++) {
        const INPUT_ROLE role = TouchOverlay_GetPositionDefaultRole(p);
        if (role < 0 || role >= INPUT_ROLE_NUMBER_OF) {
            continue;
        }
        // Find first empty slot for this role
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            M_BINDING *b = &m_Layout[INPUT_LAYOUT_DEFAULT][role].slots[s];
            if (b->pos_count == 0) {
                b->pos_count = 1;
                b->positions[0] = p;
                break;
            }
        }
    }
    // Bind meta-actions to positions that have no menu function.
    // Avoid inventory (menu_back), action (confirm), jump, look (toggle help).
    // Draw Weapon = TOUCH_DEF_TO_POS(6) = 9, Roll = TOUCH_DEF_TO_POS(5) = 8.
    m_Layout[INPUT_LAYOUT_DEFAULT][INPUT_ROLE_RESET_BINDINGS].slots[0] =
        (M_BINDING) { .pos_count = 1, .positions = { 9 } };
    m_Layout[INPUT_LAYOUT_DEFAULT][INPUT_ROLE_UNBIND_KEY].slots[0] =
        (M_BINDING) { .pos_count = 1, .positions = { 8 } };

    M_CheckConflicts(INPUT_LAYOUT_DEFAULT);

    // Copy default to all custom layouts
    for (int32_t layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
            m_Layout[layout][role] = m_Layout[INPUT_LAYOUT_DEFAULT][role];
        }
        M_CheckConflicts(layout);
    }
}

static void M_Init(void)
{
    TouchOverlay_Init();
    M_InitDefaultBindings();
}

static void M_Shutdown(void)
{
    TouchOverlay_Shutdown();
}

static void M_ProcessEvent(const SDL_Event *const event)
{
    if (event->type == SDL_FINGERDOWN
        && !g_Config.input.enable_touch_controls) {
        CONFIG_SET(g_Config.input.enable_touch_controls, true);
        TouchOverlay_SetVisible(true);
        SHOULD(Config_Write());
    }
    TouchOverlay_ProcessEvent(event);
}

static bool M_IsPressed(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
        const M_BINDING *b = M_GetBinding(layout, role, s);
        if (b->pos_count == 0) {
            continue;
        }
        bool all_pressed = true;
        for (int32_t k = 0; k < b->pos_count; k++) {
            if (!m_PosState[b->positions[k]]) {
                all_pressed = false;
                break;
            }
        }
        if (all_pressed) {
            return true;
        }
    }
    return false;
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    result->menu_skip |=
        result->menu_confirm || M_IsPressed(layout, INPUT_ROLE_ACTION);
    result->menu_confirm |= M_IsPressed(layout, INPUT_ROLE_ACTION);
    result->menu_back |= M_IsPressed(layout, INPUT_ROLE_JUMP);
    return true;
}

static bool M_IsRoleConflicted(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Conflicts[layout][role];
}

static const char *M_GetPosGlyph(const int32_t pos)
{
    if (pos < 0 || pos >= (int32_t)ARRAY_SIZE(m_PosGlyphs)) {
        return nullptr;
    }
    return m_PosGlyphs[pos];
}

static const char *M_GetName(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const M_BINDING *const b = M_GetBinding(layout, role, slot);
    const char *names[INPUT_COMBO_MAX_KEYS];
    for (int32_t k = 0; k < b->pos_count; k++) {
        names[k] = M_GetPosGlyph(b->positions[k]);
    }
    return Input_JoinKeyNames(names, b->pos_count);
}

static void M_UnassignRole(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const M_BINDING empty = { .pos_count = 0 };
    M_AssignBinding(layout, role, slot, &empty);
}

static void M_ResetLayout(const INPUT_LAYOUT layout)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        m_Layout[layout][role] = m_Layout[INPUT_LAYOUT_DEFAULT][role];
    }
    M_CheckConflicts(layout);
}

static bool M_ReadAndAssign(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    // Check which positions are currently pressed via selection mode
    const int32_t selected = TouchOverlay_GetSelectedPosition();
    if (selected >= 0) {
        if (!M_CaptureHasPos(selected)
            && m_CaptureBuffer.pos_count < INPUT_COMBO_MAX_KEYS) {
            m_CaptureBuffer.positions[m_CaptureBuffer.pos_count++] = selected;
        }
        m_CaptureActive = true;
        // Reset selection so we can detect next tap
        // (the overlay will set it again on next finger-down)
    }

    // If the first position is bound to an immediate role, assign right away.
    if (m_CaptureActive && m_CaptureBuffer.pos_count == 1
        && M_IsPositionImmediate(layout, m_CaptureBuffer.positions[0])) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.pos_count = 0;
        m_CaptureActive = false;
        return true;
    }

    // Otherwise wait for all fingers to lift (allows multi-position combos).
    if (m_CaptureActive && selected < 0 && !TouchOverlay_HasAnyFingerDown()) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.pos_count = 0;
        m_CaptureActive = false;
        return true;
    }

    return false;
}

static bool M_AssignFromJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const JSON_OBJECT *const bind_obj)
{
    const JSON_ARRAY *const combo_arr = JSON_ObjectGetArray(bind_obj, "combo");
    if (combo_arr != nullptr) {
        const int32_t count = combo_arr->length < INPUT_COMBO_MAX_KEYS
            ? (int32_t)combo_arr->length
            : INPUT_COMBO_MAX_KEYS;
        M_BINDING bind = { .pos_count = count };
        for (int32_t i = 0; i < count; i++) {
            bind.positions[i] = JSON_ArrayGetInt(combo_arr, i, -1);
        }
        M_AssignBinding(layout, role, slot, &bind);
    } else {
        const int32_t pos = JSON_ObjectGetInt(bind_obj, "position", -1);
        if (pos >= 0) {
            const M_BINDING bind = {
                .pos_count = 1,
                .positions = { pos },
            };
            M_AssignBinding(layout, role, slot, &bind);
        }
    }
    return true;
}

static bool M_AssignToJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    JSON_OBJECT *const bind_obj)
{
    const M_BINDING *user = M_GetBinding(layout, role, slot);
    const M_BINDING *def = M_GetBinding(INPUT_LAYOUT_DEFAULT, role, slot);

    if (M_BindingsEqual(user, def)
        || (user->pos_count == 0 && def->pos_count == 0)) {
        return false;
    }

    if (user->pos_count == 1) {
        JSON_ObjectAppendInt(bind_obj, "position", user->positions[0]);
    } else if (user->pos_count > 1) {
        JSON_ARRAY *const arr = JSON_ArrayNew();
        for (int32_t i = 0; i < user->pos_count; i++) {
            JSON_ArrayAppendInt(arr, user->positions[i]);
        }
        JSON_ObjectAppendArray(bind_obj, "combo", arr);
    } else {
        // Unbound: store position = -1
        JSON_ObjectAppendInt(bind_obj, "position", -1);
    }
    return true;
}

static void M_ResolveCombos(
    const INPUT_LAYOUT layout, INPUT_STATE *const result)
{
    const int32_t num_pos = TouchOverlay_GetPositionCount();

    // Update press-tick tracking: record the tick each position most recently
    // transitioned from released to held, used to detect combos that start
    // on a key the user was already holding for movement.
    m_Tick++;
    for (int32_t p = 0; p < num_pos && p < M_MAX_POSITIONS; p++) {
        if (m_PosState[p] && !m_PrefixWasHeld[p]) {
            m_PosDownTick[p] = m_Tick;
        }
    }

    // Phase 1: Collect active bindings.
    const M_BINDING *active[INPUT_ROLE_NUMBER_OF] = {};
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (InputState_GetRole(*result, role)) {
            active[role] = M_GetPressedBinding(layout, role);
        }
    }

    // Suppress combos that appear to ride on top of ongoing movement:
    // if the earliest-pressed key in the combo is bound to an immediate
    // role, the user was already performing that action when they pressed
    // the remaining keys, so the combo shortcut shouldn't fire.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr) {
            continue;
        }
        if (Input_ComboEarliestKeyIsImmediate(
                layout, M_ToCombo(active[role]), M_GetPressTick,
                M_GetComboBinding, M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            active[role] = nullptr;
        }
    }

    // Phase 2: Subset suppression — longer active combos suppress shorter.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr) {
            continue;
        }
        for (INPUT_ROLE other = 0; other < INPUT_ROLE_NUMBER_OF; other++) {
            if (other == role || active[other] == nullptr) {
                continue;
            }
            if (M_IsProperSubset(active[role], active[other])) {
                InputState_ClearRole(result, role);
                break;
            }
        }
    }

    // Phase 3: Combo deferral — if an active combo's binding is a proper
    // subset of some (not necessarily active) longer binding, defer it.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr) {
            continue;
        }
        if (((Input_IsRoleImmediate(role) || Input_IsRoleSustained(role))
             && active[role]->pos_count <= 1)
            || !Input_IsRoleRebindable(role)) {
            continue;
        }
        if (M_HasLongerCombo(layout, role, active[role])) {
            InputState_ClearRole(result, role);
            if (!m_RoleWasActive[role]) {
                m_RoleLongerFired[role] = false;
            }
            m_RoleWasActive[role] = true;
        }
    }

    // Reset prefix tracking for newly pressed positions.
    for (int32_t p = 0; p < num_pos && p < M_MAX_POSITIONS; p++) {
        if (m_PosState[p] && !m_PrefixWasHeld[p]) {
            m_PrefixComboFired[p] = false;
        }
    }

    // Phase 4: Mark longer-combo-fired state.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr || active[role]->pos_count < 2) {
            continue;
        }
        for (int32_t k = 0; k < active[role]->pos_count; k++) {
            m_PrefixComboFired[active[role]->positions[k]] = true;
        }
        for (INPUT_ROLE other = 0; other < INPUT_ROLE_NUMBER_OF; other++) {
            if (!m_RoleWasActive[other]) {
                continue;
            }
            const M_BINDING *ob = M_GetPressedBinding(layout, other);
            if (ob != nullptr && M_IsProperSubset(ob, active[role])) {
                m_RoleLongerFired[other] = true;
            }
        }
    }

    // Phase 5: Fire deferred roles on release.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (!m_RoleWasActive[role]) {
            continue;
        }
        const M_BINDING *bind = M_GetPressedBinding(layout, role);
        if (bind != nullptr) {
            continue;
        }
        if (!m_RoleLongerFired[role]) {
            InputState_SetRole(result, role, true);
        }
        m_RoleWasActive[role] = false;
        m_RoleLongerFired[role] = false;
    }

    // Phase 6: Single-position prefix deferral.
    for (int32_t p = 0; p < num_pos && p < M_MAX_POSITIONS; p++) {
        const bool held = m_PosState[p];

        if (held && M_IsComboPosition(layout, p)) {
            const INPUT_ROLE role = M_FindSinglePosRole(layout, p);
            if (role != (INPUT_ROLE)-1) {
                InputState_ClearRole(result, role);
            }
        }

        if (!held && m_PrefixWasHeld[p] && !m_PrefixComboFired[p]) {
            const INPUT_ROLE role = M_FindSinglePosRole(layout, p);
            if (role != (INPUT_ROLE)-1) {
                InputState_SetRole(result, role, true);
            }
        }

        m_PrefixWasHeld[p] = held;
    }
}

// --- Public API ---

void Touch_SetPositionState(const int32_t position, const bool pressed)
{
    const int32_t num_pos = TouchOverlay_GetPositionCount();
    if (position >= 0 && position < num_pos && position < M_MAX_POSITIONS) {
        m_PosState[position] = pressed;
    }
}

INPUT_ROLE Touch_GetPositionRole(const int32_t position)
{
    const INPUT_LAYOUT layout = g_Config.input.layout[INPUT_BACKEND_TOUCH];
    const int32_t num_pos = TouchOverlay_GetPositionCount();
    if (position < 0 || position >= num_pos) {
        return (INPUT_ROLE)-1;
    }
    for (int32_t r = 0; r < INPUT_ROLE_NUMBER_OF; r++) {
        for (int32_t s = 0; s < INPUT_BINDING_SLOTS; s++) {
            const M_BINDING *b = M_GetBinding(layout, r, s);
            if (b->pos_count == 1 && b->positions[0] == position) {
                return (INPUT_ROLE)r;
            }
        }
    }
    return (INPUT_ROLE)-1;
}

bool Touch_HasHardwareSupport(void)
{
    return SDL_GetNumTouchDevices() > 0;
}

INPUT_BACKEND_IMPL g_Input_Touch = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .discover = nullptr,
    .custom_update = M_CustomUpdate,
    .process_event = M_ProcessEvent,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = M_IsRoleConflicted,
    .get_name = M_GetName,
    .unassign_role = M_UnassignRole,
    .assign_from_json_object = M_AssignFromJSONObject,
    .assign_to_json_object = M_AssignToJSONObject,
    .reset_layout = M_ResetLayout,
    .read_and_assign = M_ReadAndAssign,
    .resolve_combos = M_ResolveCombos,
};

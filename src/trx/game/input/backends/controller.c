#include <trx/game/input/backends/controller.h>

#include <trx/core/log.h>
#include <trx/game/input/backends/internal.h>
#include <trx/game/input/combo.h>
#include <trx/game/input/names.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <string.h>

// clang-format off
#define M_ICON_X              "\\{controller button cross}"
#define M_ICON_CIRCLE         "\\{controller button circle}"
#define M_ICON_SQUARE         "\\{controller button square}"
#define M_ICON_TRIANGLE       "\\{controller button triangle}"
#define M_ICON_UP             "\\{controller dpad up}"
#define M_ICON_DOWN           "\\{controller dpad down}"
#define M_ICON_LEFT           "\\{controller dpad left}"
#define M_ICON_RIGHT          "\\{controller dpad right}"
#define M_ICON_L1             "\\{controller button l1}"
#define M_ICON_R1             "\\{controller button r1}"
#define M_ICON_L2             "\\{controller button l2}"
#define M_ICON_R2             "\\{controller button r2}"

#define M_NAME_L_STICK        "\\{controller lstick}"
#define M_NAME_L_ANALOG_UP    "\\{controller lstick up}"
#define M_NAME_L_ANALOG_RIGHT "\\{controller lstick right}"
#define M_NAME_L_ANALOG_DOWN  "\\{controller lstick down}"
#define M_NAME_L_ANALOG_LEFT  "\\{controller lstick left}"
#define M_NAME_R_STICK        "\\{controller rstick}"
#define M_NAME_R_ANALOG_UP    "\\{controller rstick up}"
#define M_NAME_R_ANALOG_RIGHT "\\{controller rstick right}"
#define M_NAME_R_ANALOG_DOWN  "\\{controller rstick down}"
#define M_NAME_R_ANALOG_LEFT  "\\{controller rstick left}"
#define M_NAME_L_TRIGGER      "\\{controller trigger left}"
#define M_NAME_R_TRIGGER      "\\{controller trigger right}"

#define M_NAME_ZL             "\\{controller button zl}"
#define M_NAME_ZR             "\\{controller button zr}"

#define M_NAME_XBOX           "\\{controller button xbox}"
#define M_NAME_PS             "\\{controller button ps}"
#define M_NAME_PS_SHARE       "\\{controller button share}"
#define M_NAME_PS_OPTIONS     "\\{controller button options}"

#define M_NAME_BACK           "\\{controller button back}"
#define M_NAME_HOME           "\\{controller button home}"
#define M_NAME_START          "\\{controller button home}"
#define M_NAME_SHARE          "\\{controller button share}"
#define M_NAME_CAPTURE        "\\{controller button capture}"
#define M_NAME_TOUCHPAD       "\\{controller button touchpad}"
#define M_NAME_MIC            "\\{controller button mic}"
#define M_NAME_PADDLE_1       "\\{controller button paddle 1}"
#define M_NAME_PADDLE_2       "\\{controller button paddle 2}"
#define M_NAME_PADDLE_3       "\\{controller button paddle 3}"
#define M_NAME_PADDLE_4       "\\{controller button paddle 4}"

#define M_NAME_L_BUMPER       "\\{controller bumper left}"
#define M_NAME_R_BUMPER       "\\{controller bumper right}"

#define M_NAME_A              "\\{controller button a}"
#define M_NAME_B              "\\{controller button b}"
#define M_NAME_X              "\\{controller button x}"
#define M_NAME_Y              "\\{controller button y}"
// clang-format on

// Per-button/axis tracking for combo prefix deferral.
// Index: buttons use their enum directly, axes use
// SDL_CONTROLLER_BUTTON_MAX + axis*2 + (axis_dir == 1 ? 1 : 0).
#define M_PREFIX_SLOTS (SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_MAX * 2)

typedef enum {
    BT_BUTTON = 0,
    BT_AXIS = 1,
} BUTTON_TYPE;

typedef struct {
    BUTTON_TYPE type;
    union {
        SDL_GameControllerButton button;
        SDL_GameControllerAxis axis;
    } bind;
    int16_t axis_dir;
} CONTROLLER_MAP;

typedef struct {
    INPUT_ROLE role;
    CONTROLLER_MAP map;
} BUILTIN_CONTROLLER_LAYOUT;

typedef struct {
    int32_t key_count;
    CONTROLLER_MAP keys[INPUT_COMBO_MAX_KEYS];
} CONTROLLER_BINDING;

typedef struct {
    CONTROLLER_BINDING slots[INPUT_BINDING_SLOTS];
} CONTROLLER_ROLE_BINDING;

static BUILTIN_CONTROLLER_LAYOUT m_BuiltinLayout[] = {
#define INPUT_CONTROLLER_ASSIGN_BUTTON(role, bind)                             \
    { role, { BT_BUTTON, { .button = bind }, 0 } },
#define INPUT_CONTROLLER_ASSIGN_AXIS(role, bind, axis_dir)                     \
    { role, { BT_AXIS, { .axis = bind }, axis_dir } },
#include <trx/game/input/backends/controller.def>
    // guard
    { -1, { 0, { 0 }, 0 } },
};

static CONTROLLER_ROLE_BINDING m_Layout[INPUT_LAYOUT_NUMBER_OF]
                                       [INPUT_ROLE_NUMBER_OF];

static SDL_GameController *m_Controller = nullptr;
static const char *m_ControllerName = nullptr;
static SDL_GameControllerType m_ControllerType = SDL_CONTROLLER_TYPE_UNKNOWN;

static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {};

// Internal controller state tables updated via SDL events
static bool m_ButtonState[SDL_CONTROLLER_BUTTON_MAX] = {};
static int16_t m_AxisState[SDL_CONTROLLER_AXIS_MAX] = {};

static bool m_PrefixWasHeld[M_PREFIX_SLOTS];
static bool m_PrefixComboFired[M_PREFIX_SLOTS];

// Per-input press-tick table, indexed the same way as the prefix arrays.
static uint32_t m_InputDownTick[M_PREFIX_SLOTS];
static uint32_t m_Tick;

// Per-role deferral tracking for combo disambiguation.
static bool m_RoleWasActive[INPUT_ROLE_NUMBER_OF];

static bool m_RoleLongerFired[INPUT_ROLE_NUMBER_OF];

// Combo capture state for listen mode.
static CONTROLLER_BINDING m_CaptureBuffer = { .key_count = 0 };

static bool m_CaptureActive = false;

static const char *M_GetButtonName(const SDL_GameControllerButton button)
{
    // First switch: Handle platform-specific deviations from defaults
    switch (m_ControllerType) {
    case SDL_CONTROLLER_TYPE_PS3:
    case SDL_CONTROLLER_TYPE_PS4:
    case SDL_CONTROLLER_TYPE_PS5:
        // clang-format off
        switch (button) {
        case SDL_CONTROLLER_BUTTON_A:             return M_ICON_X;
        case SDL_CONTROLLER_BUTTON_B:             return M_ICON_CIRCLE;
        case SDL_CONTROLLER_BUTTON_X:             return M_ICON_SQUARE;
        case SDL_CONTROLLER_BUTTON_Y:             return M_ICON_TRIANGLE;
        case SDL_CONTROLLER_BUTTON_BACK:          return M_NAME_PS_OPTIONS;
        case SDL_CONTROLLER_BUTTON_START:         return M_NAME_PS_SHARE;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return M_NAME_L_STICK;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return M_NAME_R_STICK;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return M_ICON_L1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return M_ICON_R1;
        case SDL_CONTROLLER_BUTTON_MISC1:         return M_NAME_MIC;
        case SDL_CONTROLLER_BUTTON_GUIDE:         return M_NAME_PS;
        default: break;
        }
        // clang-format on
        break;

    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        // clang-format off
        switch (button) {
        case SDL_CONTROLLER_BUTTON_A:             return M_NAME_B;
        case SDL_CONTROLLER_BUTTON_B:             return M_NAME_A;
        case SDL_CONTROLLER_BUTTON_X:             return M_NAME_Y;
        case SDL_CONTROLLER_BUTTON_Y:             return M_NAME_X;
        case SDL_CONTROLLER_BUTTON_START:         return M_NAME_START;
        case SDL_CONTROLLER_BUTTON_MISC1:         return M_NAME_CAPTURE;
        default: break;
        }
        // clang-format on
        break;

    case SDL_CONTROLLER_TYPE_XBOX360:
    case SDL_CONTROLLER_TYPE_XBOXONE:
        // clang-format off
        switch (button) {
        case SDL_CONTROLLER_BUTTON_GUIDE:         return M_NAME_XBOX;
        default: break;
        }
        // clang-format on
        break;

    default:
        break;
    }

    // Second switch: Provide default mappings for all keys
    switch (button) {
    case SDL_CONTROLLER_BUTTON_INVALID:
    case SDL_CONTROLLER_BUTTON_MAX:
        return nullptr;

        // clang-format off
    case SDL_CONTROLLER_BUTTON_A:             return M_NAME_A;
    case SDL_CONTROLLER_BUTTON_B:             return M_NAME_B;
    case SDL_CONTROLLER_BUTTON_X:             return M_NAME_X;
    case SDL_CONTROLLER_BUTTON_Y:             return M_NAME_Y;
    case SDL_CONTROLLER_BUTTON_BACK:          return M_NAME_BACK;
    case SDL_CONTROLLER_BUTTON_GUIDE:         return M_NAME_HOME;
    case SDL_CONTROLLER_BUTTON_START:         return M_NAME_START;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return M_NAME_L_STICK;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return M_NAME_R_STICK;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return M_NAME_L_BUMPER;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return M_NAME_R_BUMPER;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:       return M_ICON_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return M_ICON_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return M_ICON_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return M_ICON_RIGHT;
    case SDL_CONTROLLER_BUTTON_MISC1:         return M_NAME_SHARE;
    case SDL_CONTROLLER_BUTTON_PADDLE1:       return M_NAME_PADDLE_1;
    case SDL_CONTROLLER_BUTTON_PADDLE2:       return M_NAME_PADDLE_2;
    case SDL_CONTROLLER_BUTTON_PADDLE3:       return M_NAME_PADDLE_3;
    case SDL_CONTROLLER_BUTTON_PADDLE4:       return M_NAME_PADDLE_4;
    case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return M_NAME_TOUCHPAD;
        // clang-format on

    default:
        return "????";
    }
}

// Update internal controller button/axis state from SDL events.
// @param event     Event to process.
static void M_ProcessEvent(const SDL_Event *const event)
{
    switch (event->type) {
    case SDL_CONTROLLERBUTTONDOWN:
        m_ButtonState[event->cbutton.button] = true;
        break;
    case SDL_CONTROLLERBUTTONUP:
        m_ButtonState[event->cbutton.button] = false;
        break;
    case SDL_CONTROLLERAXISMOTION: {
        const Sint16 value = event->caxis.value;
        if (value < -SDL_JOYSTICK_AXIS_MAX / 2) {
            m_AxisState[event->caxis.axis] = -1;
        } else if (value > SDL_JOYSTICK_AXIS_MAX / 2) {
            m_AxisState[event->caxis.axis] = 1;
        } else {
            m_AxisState[event->caxis.axis] = 0;
        }
        break;
    }
    default:
        break;
    }
}

static bool M_JoyBtn(const SDL_GameControllerButton button)
{
    if (m_Controller == nullptr || button == SDL_CONTROLLER_BUTTON_INVALID) {
        return false;
    }
    return m_ButtonState[button];
}

static int16_t M_JoyAxis(const SDL_GameControllerAxis axis)
{
    if (m_Controller == nullptr || axis == SDL_CONTROLLER_AXIS_INVALID) {
        return false;
    }
    return m_AxisState[axis];
}

static bool M_CheckMap(const CONTROLLER_MAP *const map)
{
    if (map->type == BT_BUTTON) {
        return M_JoyBtn(map->bind.button);
    } else {
        return M_JoyAxis(map->bind.axis) == map->axis_dir;
    }
}

static bool M_CheckBinding(const CONTROLLER_BINDING *const bind)
{
    if (bind->key_count == 0) {
        return false;
    }
    for (int32_t k = 0; k < bind->key_count; k++) {
        if (!M_CheckMap(&bind->keys[k])) {
            return false;
        }
    }
    return true;
}

static const CONTROLLER_BINDING *M_GetBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    return &m_Layout[layout][role].slots[slot];
}

static INPUT_COMBO_BINDING M_GetComboBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING *b = M_GetBinding(layout, role, slot);
    return (INPUT_COMBO_BINDING) {
        .key_count = b->key_count,
        .keys = b->keys,
        .key_stride = sizeof(CONTROLLER_MAP),
    };
}

static bool M_MapsEqual(
    const CONTROLLER_MAP *const a, const CONTROLLER_MAP *const b)
{
    if (a->type != b->type) {
        return false;
    }
    if (a->type == BT_BUTTON) {
        return a->bind.button == b->bind.button;
    }
    return a->bind.axis == b->bind.axis && a->axis_dir == b->axis_dir;
}

static bool M_ComboKeysEqual(const void *const a, const void *const b)
{
    return M_MapsEqual((const CONTROLLER_MAP *)a, (const CONTROLLER_MAP *)b);
}

static bool M_GetBindState(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
        const CONTROLLER_BINDING *bind = &m_Layout[layout][role].slots[slot];
        if (bind->key_count >= 2
            && Input_ComboIsKeyImmediate(
                layout, &bind->keys[0], M_GetComboBinding, M_ComboKeysEqual)) {
            continue;
        }
        if (M_CheckBinding(bind)) {
            return true;
        }
    }
    return false;
}

static bool M_BindingsEqual(
    const CONTROLLER_BINDING *const a, const CONTROLLER_BINDING *const b)
{
    if (a->key_count != b->key_count || a->key_count == 0) {
        return false;
    }
    for (int32_t i = 0; i < a->key_count; i++) {
        if (!M_MapsEqual(&a->keys[i], &b->keys[i])) {
            return false;
        }
    }
    return true;
}

static bool M_CheckConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role1, const INPUT_ROLE role2)
{
    for (int32_t s1 = 0; s1 < INPUT_BINDING_SLOTS; s1++) {
        const CONTROLLER_BINDING *b1 = M_GetBinding(layout, role1, s1);
        if (b1->key_count == 0) {
            continue;
        }
        for (int32_t s2 = 0; s2 < INPUT_BINDING_SLOTS; s2++) {
            const CONTROLLER_BINDING *b2 = M_GetBinding(layout, role2, s2);
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

static void M_CheckConflicts(const INPUT_LAYOUT layout)
{
    Input_ConflictHelper(layout, M_CheckConflict, M_AssignConflict);
    Input_ComboCheckConflicts(
        layout, M_GetComboBinding, M_ComboKeysEqual, m_Conflicts[layout]);
}

static int16_t M_GetAssignedButtonType(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING *bind = M_GetBinding(layout, role, slot);
    return bind->key_count > 0 ? bind->keys[0].type : BT_BUTTON;
}

static int16_t M_GetAssignedBind(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING *bind = M_GetBinding(layout, role, slot);
    if (bind->key_count == 0) {
        return SDL_CONTROLLER_BUTTON_INVALID;
    }
    const CONTROLLER_MAP *map = &bind->keys[0];
    if (map->type == BT_BUTTON) {
        return map->bind.button;
    } else {
        return map->bind.axis;
    }
}

static int16_t M_GetAssignedAxisDir(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING *bind = M_GetBinding(layout, role, slot);
    return bind->key_count > 0 ? bind->keys[0].axis_dir : 0;
}

static void M_AssignBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const CONTROLLER_BINDING *const bind)
{
    m_Layout[layout][role].slots[slot] = *bind;
    M_CheckConflicts(layout);
}

static void M_AssignButton(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const int16_t button)
{
    const CONTROLLER_BINDING bind = {
        .key_count = button != SDL_CONTROLLER_BUTTON_INVALID ? 1 : 0,
        .keys = { { BT_BUTTON, { .button = button }, 0 } },
    };
    M_AssignBinding(layout, role, slot, &bind);
}

static void M_AssignAxis(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const int16_t axis, const int16_t axis_dir)
{
    const CONTROLLER_BINDING bind = {
        .key_count = 1,
        .keys = { { BT_AXIS, { .axis = axis }, axis_dir } },
    };
    M_AssignBinding(layout, role, slot, &bind);
}

static SDL_GameController *M_FindController(void)
{
    if (m_Controller != nullptr) {
        return m_Controller;
    }

    int32_t controllers = SDL_NumJoysticks();
    LOG_INFO("%d controllers", controllers);
    for (int32_t i = 0; i < controllers; i++) {
        m_ControllerName = SDL_GameControllerNameForIndex(i);
        m_ControllerType = SDL_GameControllerTypeForIndex(i);
        bool is_game_controller = SDL_IsGameController(i);
        LOG_DEBUG(
            "controller %d: %s %d (%d)", i, m_ControllerName, m_ControllerType,
            is_game_controller);
        if (is_game_controller) {
            SDL_GameController *const result = SDL_GameControllerOpen(i);
            if (result == nullptr) {
                LOG_ERROR("Could not open controller: %s", SDL_GetError());
            }
            return result;
        }
    }

    return nullptr;
}

static void M_ResetLayout(const INPUT_LAYOUT layout)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        m_Layout[layout][role] = m_Layout[INPUT_LAYOUT_DEFAULT][role];
    }
    M_CheckConflicts(layout);
}

static void M_Discover(void)
{
    if (m_Controller != nullptr) {
        SDL_GameControllerClose(m_Controller);
        m_Controller = nullptr;
    }
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR) < 0) {
        LOG_ERROR("Error while calling SDL_InitSubSystem: %s", SDL_GetError());
        return;
    }
    m_Controller = M_FindController();
}

static void M_Init(void)
{
    // first, reset all roles to unbound
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
            m_Layout[INPUT_LAYOUT_DEFAULT][role].slots[slot] =
                (CONTROLLER_BINDING) { .key_count = 0 };
        }
    }
    // then load the defined default bindings into slot 0
    for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
        const BUILTIN_CONTROLLER_LAYOUT *const builtin = &m_BuiltinLayout[i];
        m_Layout[INPUT_LAYOUT_DEFAULT][builtin->role].slots[0] =
            (CONTROLLER_BINDING) {
                .key_count = 1,
                .keys = { builtin->map },
            };
    }
    M_CheckConflicts(INPUT_LAYOUT_DEFAULT);

    for (int32_t layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_ResetLayout(layout);
    }
}

static void M_Shutdown(void)
{
    if (m_Controller != nullptr) {
        SDL_GameControllerClose(m_Controller);
        m_Controller = nullptr;
    }
    memset(m_ButtonState, 0, sizeof(m_ButtonState));
    memset(m_AxisState, 0, sizeof(m_AxisState));
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    if (m_Controller == nullptr) {
        return false;
    }
    result->menu_back |= M_JoyBtn(SDL_CONTROLLER_BUTTON_Y);
    result->menu_skip |= result->menu_confirm;
    return true;
}

static bool M_IsPressed(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return M_GetBindState(layout, role);
}

static const char *M_GetAxisName(
    const SDL_GameControllerAxis axis, const int16_t axis_dir)
{
    // clang-format off
    switch (m_ControllerType) {
        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return nullptr;
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_ICON_L2;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_ICON_R2;
                case SDL_CONTROLLER_AXIS_MAX:             return nullptr;
            }
            break;

        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return nullptr;
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_NAME_ZL;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_NAME_ZR;
                case SDL_CONTROLLER_AXIS_MAX:             return nullptr;
            }
            break;

        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
        default:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return nullptr;
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_NAME_L_TRIGGER;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_NAME_R_TRIGGER;
                case SDL_CONTROLLER_AXIS_MAX:             return nullptr;
            }
            break;

    }
    // clang-format on
    return nullptr;
}

static bool M_IsRoleConflicted(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Conflicts[layout][role];
}

static const char *M_GetMapName(const CONTROLLER_MAP *const map)
{
    if (map->type == BT_BUTTON) {
        return M_GetButtonName(map->bind.button);
    } else {
        return M_GetAxisName(map->bind.axis, map->axis_dir);
    }
}

static const char *M_GetName(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING *const bind = M_GetBinding(layout, role, slot);
    const char *names[INPUT_COMBO_MAX_KEYS];
    for (int32_t k = 0; k < bind->key_count; k++) {
        names[k] = M_GetMapName(&bind->keys[k]);
    }
    return Input_JoinKeyNames(names, bind->key_count);
}

static void M_UnassignRole(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    const CONTROLLER_BINDING empty = { .key_count = 0 };
    M_AssignBinding(layout, role, slot, &empty);
}

static bool M_AssignFromJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    const JSON_OBJECT *const bind_obj)
{
    const JSON_ARRAY *const combo_arr = JSON_ObjectGetArray(bind_obj, "combo");
    if (combo_arr != nullptr) {
        // New combo format: "combo": [{button_type, bind, axis_dir}, ...]
        const int32_t count = combo_arr->length < INPUT_COMBO_MAX_KEYS
            ? (int32_t)combo_arr->length
            : INPUT_COMBO_MAX_KEYS;
        CONTROLLER_BINDING cb = { .key_count = count };
        for (int32_t i = 0; i < count; i++) {
            const JSON_OBJECT *const key_obj =
                JSON_ArrayGetObject(combo_arr, i);
            cb.keys[i].type =
                JSON_ObjectGetInt(key_obj, "button_type", BT_BUTTON);
            const int16_t b = JSON_ObjectGetInt(
                key_obj, "bind", SDL_CONTROLLER_BUTTON_INVALID);
            if (cb.keys[i].type == BT_BUTTON) {
                cb.keys[i].bind.button = b;
            } else {
                cb.keys[i].bind.axis = b;
            }
            cb.keys[i].axis_dir = JSON_ObjectGetInt(key_obj, "axis_dir", 0);
        }
        M_AssignBinding(layout, role, slot, &cb);
    } else {
        // Legacy single-key format
        int16_t button_type = M_GetAssignedButtonType(layout, role, slot);
        button_type = JSON_ObjectGetInt(bind_obj, "button_type", button_type);

        int16_t bind = M_GetAssignedBind(layout, role, slot);
        bind = JSON_ObjectGetInt(bind_obj, "bind", bind);

        int16_t axis_dir = M_GetAssignedAxisDir(layout, role, slot);
        axis_dir = JSON_ObjectGetInt(bind_obj, "axis_dir", axis_dir);

        if (button_type == BT_BUTTON) {
            M_AssignButton(layout, role, slot, bind);
        } else {
            M_AssignAxis(layout, role, slot, bind, axis_dir);
        }
    }
    return true;
}

static bool M_AssignToJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot,
    JSON_OBJECT *const bind_obj)
{
    const CONTROLLER_BINDING *user = M_GetBinding(layout, role, slot);
    const CONTROLLER_BINDING *def =
        M_GetBinding(INPUT_LAYOUT_DEFAULT, role, slot);

    if (M_BindingsEqual(user, def)
        || (user->key_count == 0 && def->key_count == 0)) {
        return false;
    }

    if (user->key_count == 0) {
        // Explicitly unbound
        JSON_ObjectAppendInt(bind_obj, "button_type", (int16_t)BT_BUTTON);
        JSON_ObjectAppendInt(
            bind_obj, "bind", (int16_t)SDL_CONTROLLER_BUTTON_INVALID);
        JSON_ObjectAppendInt(bind_obj, "axis_dir", (int16_t)0);
    } else if (user->key_count == 1) {
        // Single key (legacy format for backward compat)
        const CONTROLLER_MAP *map = &user->keys[0];
        JSON_ObjectAppendInt(bind_obj, "button_type", map->type);
        JSON_ObjectAppendInt(
            bind_obj, "bind",
            map->type == BT_BUTTON ? map->bind.button : map->bind.axis);
        JSON_ObjectAppendInt(bind_obj, "axis_dir", map->axis_dir);
    } else {
        // Multi-key combo
        JSON_ARRAY *const arr = JSON_ArrayNew();
        for (int32_t i = 0; i < user->key_count; i++) {
            const CONTROLLER_MAP *map = &user->keys[i];
            JSON_OBJECT *const key_obj = JSON_ObjectNew();
            JSON_ObjectAppendInt(key_obj, "button_type", map->type);
            JSON_ObjectAppendInt(
                key_obj, "bind",
                map->type == BT_BUTTON ? map->bind.button : map->bind.axis);
            JSON_ObjectAppendInt(key_obj, "axis_dir", map->axis_dir);
            JSON_ArrayAppendObject(arr, key_obj);
        }
        JSON_ObjectAppendArray(bind_obj, "combo", arr);
    }
    return true;
}

static int32_t M_MapToPrefixIdx(const CONTROLLER_MAP *const map)
{
    if (map->type == BT_BUTTON) {
        return map->bind.button;
    }
    return SDL_CONTROLLER_BUTTON_MAX + map->bind.axis * 2
        + (map->axis_dir == 1 ? 1 : 0);
}

static uint32_t M_GetPressTick(const void *const key)
{
    return m_InputDownTick[M_MapToPrefixIdx((const CONTROLLER_MAP *)key)];
}

static bool M_IsInputHeld(const CONTROLLER_MAP *const map)
{
    return M_CheckMap(map);
}

static INPUT_COMBO_BINDING M_ToCombo(const CONTROLLER_BINDING *const b)
{
    return (INPUT_COMBO_BINDING) {
        .key_count = b->key_count,
        .keys = b->keys,
        .key_stride = sizeof(CONTROLLER_MAP),
    };
}

static const CONTROLLER_BINDING *M_GetPressedBinding(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (int32_t slot = 0; slot < INPUT_BINDING_SLOTS; slot++) {
        const CONTROLLER_BINDING *bind = M_GetBinding(layout, role, slot);
        if (M_CheckBinding(bind)) {
            return bind;
        }
    }
    return nullptr;
}

static void M_ResolvePrefixDeferral(
    const INPUT_LAYOUT layout, INPUT_STATE *const result,
    const CONTROLLER_MAP *const map)
{
    const int32_t idx = M_MapToPrefixIdx(map);
    const bool held = M_IsInputHeld(map);

    if (held
        && Input_ComboIsStarter(
            layout, map, M_GetComboBinding, M_ComboKeysEqual)) {
        const INPUT_ROLE role = Input_ComboFindDeferrableRole(
            layout, map, M_GetComboBinding, M_ComboKeysEqual);
        if (role != (INPUT_ROLE)-1) {
            InputState_ClearRole(result, role);
        }
    }

    if (!held && m_PrefixWasHeld[idx] && !m_PrefixComboFired[idx]) {
        const INPUT_ROLE role = Input_ComboFindDeferrableRole(
            layout, map, M_GetComboBinding, M_ComboKeysEqual);
        if (role != (INPUT_ROLE)-1) {
            InputState_SetRole(result, role, true);
        }
    }

    m_PrefixWasHeld[idx] = held;
}

static void M_ResolveCombos(
    const INPUT_LAYOUT layout, INPUT_STATE *const result)
{
    // Update press-tick tracking for every input, mirroring the prefix
    // tables' index layout.
    m_Tick++;
    for (SDL_GameControllerButton btn = 0; btn < SDL_CONTROLLER_BUTTON_MAX;
         btn++) {
        const CONTROLLER_MAP map = { BT_BUTTON, { .button = btn }, 0 };
        const int32_t idx = M_MapToPrefixIdx(&map);
        if (M_JoyBtn(btn) && !m_PrefixWasHeld[idx]) {
            m_InputDownTick[idx] = m_Tick;
        }
    }
    for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
         axis++) {
        for (int16_t dir = -1; dir <= 1; dir += 2) {
            const CONTROLLER_MAP map = { BT_AXIS, { .axis = axis }, dir };
            const int32_t idx = M_MapToPrefixIdx(&map);
            if (M_JoyAxis(axis) == dir && !m_PrefixWasHeld[idx]) {
                m_InputDownTick[idx] = m_Tick;
            }
        }
    }

    // Phase 1: Collect active bindings.
    const CONTROLLER_BINDING *active[INPUT_ROLE_NUMBER_OF] = {};
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (InputState_GetRole(*result, role)) {
            active[role] = M_GetPressedBinding(layout, role);
        }
    }

    // Suppress invalid combos (non-capturing sustained + immediate).
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] != nullptr
            && Input_ComboSustainedHasImmediate(
                layout, M_ToCombo(active[role]), M_GetComboBinding,
                M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            active[role] = nullptr;
        }
    }

    // Suppress combos that appear to ride on top of ongoing movement:
    // if the earliest-pressed key in the combo is bound to an immediate
    // role, the user was already performing that action when they pressed
    // the remaining combo keys.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] != nullptr
            && Input_ComboEarliestKeyIsImmediate(
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
            if (Input_ComboIsProperSubset(
                    M_ToCombo(active[role]), M_ToCombo(active[other]),
                    M_ComboKeysEqual)) {
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
             && active[role]->key_count <= 1)
            || !Input_IsRoleRebindable(role)) {
            continue;
        }
        if (Input_ComboHasLonger(
                layout, role, M_ToCombo(active[role]), M_GetComboBinding,
                M_ComboKeysEqual)) {
            InputState_ClearRole(result, role);
            if (!m_RoleWasActive[role]) {
                m_RoleLongerFired[role] = false;
            }
            m_RoleWasActive[role] = true;
        }
    }

    // Reset prefix tracking for newly pressed inputs.
    for (SDL_GameControllerButton btn = 0; btn < SDL_CONTROLLER_BUTTON_MAX;
         btn++) {
        const int32_t idx = (int32_t)btn;
        if (M_JoyBtn(btn) && !m_PrefixWasHeld[idx]) {
            m_PrefixComboFired[idx] = false;
        }
    }
    for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
         axis++) {
        for (int16_t dir = -1; dir <= 1; dir += 2) {
            const CONTROLLER_MAP map = { BT_AXIS, { .axis = axis }, dir };
            const int32_t idx = M_MapToPrefixIdx(&map);
            if (M_JoyAxis(axis) == dir && !m_PrefixWasHeld[idx]) {
                m_PrefixComboFired[idx] = false;
            }
        }
    }

    // Phase 4: Mark longer-combo-fired state.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (active[role] == nullptr || active[role]->key_count < 2) {
            continue;
        }
        for (int32_t k = 0; k < active[role]->key_count; k++) {
            const int32_t idx = M_MapToPrefixIdx(&active[role]->keys[k]);
            m_PrefixComboFired[idx] = true;
        }
        for (INPUT_ROLE other = 0; other < INPUT_ROLE_NUMBER_OF; other++) {
            if (!m_RoleWasActive[other]) {
                continue;
            }
            const CONTROLLER_BINDING *ob = M_GetPressedBinding(layout, other);
            if (ob != nullptr
                && Input_ComboIsProperSubset(
                    M_ToCombo(ob), M_ToCombo(active[role]), M_ComboKeysEqual)) {
                m_RoleLongerFired[other] = true;
            }
        }
    }

    // Phase 5: Fire deferred roles on release.
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        if (!m_RoleWasActive[role]) {
            continue;
        }
        const CONTROLLER_BINDING *bind = M_GetPressedBinding(layout, role);
        if (bind != nullptr) {
            continue;
        }
        if (!m_RoleLongerFired[role]) {
            InputState_SetRole(result, role, true);
        }
        m_RoleWasActive[role] = false;
        m_RoleLongerFired[role] = false;
    }

    // Phase 6: Single-input prefix deferral.
    for (SDL_GameControllerButton btn = 0; btn < SDL_CONTROLLER_BUTTON_MAX;
         btn++) {
        const CONTROLLER_MAP map = { BT_BUTTON, { .button = btn }, 0 };
        M_ResolvePrefixDeferral(layout, result, &map);
    }
    for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
         axis++) {
        for (int16_t dir = -1; dir <= 1; dir += 2) {
            const CONTROLLER_MAP map = { BT_AXIS, { .axis = axis }, dir };
            M_ResolvePrefixDeferral(layout, result, &map);
        }
    }
}

static bool M_CaptureHasMap(const CONTROLLER_MAP *const map)
{
    for (int32_t i = 0; i < m_CaptureBuffer.key_count; i++) {
        if (M_MapsEqual(&m_CaptureBuffer.keys[i], map)) {
            return true;
        }
    }
    return false;
}

static bool M_ReadAndAssign(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int32_t slot)
{
    // Count currently held inputs and accumulate new ones into the buffer.
    bool any_held = false;
    for (SDL_GameControllerButton button = 0;
         button < SDL_CONTROLLER_BUTTON_MAX; button++) {
        if (!M_JoyBtn(button)) {
            continue;
        }
        any_held = true;
        const CONTROLLER_MAP map = { BT_BUTTON, { .button = button }, 0 };
        if (!M_CaptureHasMap(&map)
            && m_CaptureBuffer.key_count < INPUT_COMBO_MAX_KEYS) {
            m_CaptureBuffer.keys[m_CaptureBuffer.key_count++] = map;
        }
        m_CaptureActive = true;
    }
    for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
         axis++) {
        const int16_t axis_dir = M_JoyAxis(axis);
        if (axis_dir == 0) {
            continue;
        }
        any_held = true;
        const CONTROLLER_MAP map = { BT_AXIS, { .axis = axis }, axis_dir };
        if (!M_CaptureHasMap(&map)
            && m_CaptureBuffer.key_count < INPUT_COMBO_MAX_KEYS) {
            m_CaptureBuffer.keys[m_CaptureBuffer.key_count++] = map;
        }
        m_CaptureActive = true;
    }

    // If the first input captured is bound to an immediate role (movement,
    // action, etc.), assign as single key right away — don't wait for combo.
    if (m_CaptureActive && m_CaptureBuffer.key_count == 1 && any_held
        && Input_ComboIsKeyImmediate(
            layout, &m_CaptureBuffer.keys[0], M_GetComboBinding,
            M_ComboKeysEqual)) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.key_count = 0;
        m_CaptureActive = false;
        return true;
    }

    // All inputs released after at least one was captured — assign the chord.
    if (!any_held && m_CaptureActive) {
        M_AssignBinding(layout, role, slot, &m_CaptureBuffer);
        m_CaptureBuffer.key_count = 0;
        m_CaptureActive = false;
        return true;
    }
    return false;
}

INPUT_BACKEND_IMPL g_Input_Controller = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .discover = M_Discover,
    .process_event = M_ProcessEvent,
    .custom_update = M_CustomUpdate,
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

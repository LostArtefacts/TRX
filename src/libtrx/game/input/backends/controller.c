#include "game/input/backends/controller.h"

#include "game/input/backends/internal.h"
#include "log.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

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

static BUILTIN_CONTROLLER_LAYOUT m_BuiltinLayout[] = {
#define INPUT_CONTROLLER_ASSIGN_BUTTON(role, bind)                             \
    { role, { BT_BUTTON, { .button = bind }, 0 } },
#define INPUT_CONTROLLER_ASSIGN_AXIS(role, bind, axis_dir)                     \
    { role, { BT_AXIS, { .axis = bind }, axis_dir } },
#include "game/input/backends/controller.def"
    // guard
    { -1, { 0, { 0 }, 0 } },
};

#define M_ICON_X "\\{button x}"
#define M_ICON_CIRCLE "\\{button circle}"
#define M_ICON_SQUARE "\\{button square}"
#define M_ICON_TRIANGLE "\\{button triangle}"
#define M_ICON_UP "\\{button up} "
#define M_ICON_DOWN "\\{button down} "
#define M_ICON_LEFT "\\{button left} "
#define M_ICON_RIGHT "\\{button right} "
#define M_ICON_L1 "\\{button l1}"
#define M_ICON_R1 "\\{button r1}"
#define M_ICON_L2 "\\{button l2}"
#define M_ICON_R2 "\\{button r2}"

// TODO: replace all of the text with icons
#define M_NAME_L_ANALOG_LEFT "LSTK" M_ICON_LEFT
#define M_NAME_L_ANALOG_UP "LSTK" M_ICON_UP
#define M_NAME_L_ANALOG_RIGHT "LSTK" M_ICON_RIGHT
#define M_NAME_L_ANALOG_DOWN "LSTK" M_ICON_DOWN
#define M_NAME_R_ANALOG_LEFT "RSTK" M_ICON_LEFT
#define M_NAME_R_ANALOG_UP "RSTK" M_ICON_UP
#define M_NAME_R_ANALOG_RIGHT "RSTK" M_ICON_RIGHT
#define M_NAME_R_ANALOG_DOWN "RSTK" M_ICON_DOWN
#define M_NAME_L_TRIGGER "LTRIG"
#define M_NAME_R_TRIGGER "RTRIG"
#define M_NAME_ZL "ZL"

#define M_NAME_ZR "ZR"
#define M_NAME_BACK "BACK"

#define M_NAME_CAPTURE "CAPTR"
#define M_NAME_CREATE "CREAT"

#define M_NAME_HOME "HOME"
#define M_NAME_XBOX "XBOX"
#define M_NAME_START "START"
#define M_NAME_SHARE "SHARE"

#define M_NAME_TOUCHPAD "TOUCH"

#define M_NAME_L3 "L3"
#define M_NAME_R3 "R3"
#define M_NAME_MIC "MIC"
#define M_NAME_PADDLE_1 "PADDLE 1"
#define M_NAME_PADDLE_2 "PADDLE 2"
#define M_NAME_PADDLE_3 "PADDLE 3"
#define M_NAME_PADDLE_4 "PADDLE 4"

#define M_NAME_L_STICK "LSTK"
#define M_NAME_R_STICK "RSTK"
#define M_NAME_L_BUMPER "LBUMP"
#define M_NAME_R_BUMPER "RBUMP"

#define M_NAME_A "A"
#define M_NAME_B "B"
#define M_NAME_X "X"
#define M_NAME_Y "Y"

static CONTROLLER_MAP m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF];

static SDL_GameController *m_Controller = nullptr;
static const char *m_ControllerName = nullptr;
static SDL_GameControllerType m_ControllerType = SDL_CONTROLLER_TYPE_UNKNOWN;

static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {};

static const char *M_GetButtonName(SDL_GameControllerButton button);
static const char *M_GetAxisName(SDL_GameControllerAxis axis, int16_t axis_dir);

static bool M_JoyBtn(SDL_GameControllerButton button);
static int16_t M_JoyAxis(SDL_GameControllerAxis axis);
static bool M_GetBindState(INPUT_LAYOUT layout, INPUT_ROLE role);

static int16_t M_GetUniqueBind(INPUT_LAYOUT layout, INPUT_ROLE role);
static int16_t M_GetAssignedButtonType(INPUT_LAYOUT layout, INPUT_ROLE role);
static int16_t M_GetAssignedBind(INPUT_LAYOUT layout, INPUT_ROLE role);
static int16_t M_GetAssignedAxisDir(INPUT_LAYOUT layout, INPUT_ROLE role);
static void M_AssignButton(
    INPUT_LAYOUT layout, INPUT_ROLE role, int16_t button);
static void M_AssignAxis(
    INPUT_LAYOUT layout, INPUT_ROLE role, int16_t axis, int16_t axis_dir);
static bool M_CheckConflict(
    INPUT_LAYOUT layout, INPUT_ROLE role1, INPUT_ROLE role2);
static void M_AssignConflict(
    INPUT_LAYOUT layout, INPUT_ROLE role, bool conflict);
static void M_CheckConflicts(INPUT_LAYOUT layout);
static SDL_GameController *M_FindController(void);

static void M_Init(void);
static void M_Shutdown(void);
static void M_Discover(void);
static bool M_CustomUpdate(INPUT_STATE *result, INPUT_LAYOUT layout);
static bool M_IsPressed(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_IsRoleConflicted(INPUT_LAYOUT layout, INPUT_ROLE role);
static const char *M_GetName(INPUT_LAYOUT layout, INPUT_ROLE role);
static void M_UnassignRole(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_AssignFromJSONObject(
    INPUT_LAYOUT layout, INPUT_ROLE role, JSON_OBJECT *bind_obj);
static bool M_AssignToJSONObject(
    INPUT_LAYOUT layout, INPUT_ROLE role, JSON_OBJECT *bind_obj);
static void M_ResetLayout(INPUT_LAYOUT layout);
static bool M_ReadAndAssign(INPUT_LAYOUT layout, INPUT_ROLE role);

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
        case SDL_CONTROLLER_BUTTON_BACK:          return M_NAME_CREATE;
        case SDL_CONTROLLER_BUTTON_START:         return M_NAME_START;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return M_NAME_L3;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return M_NAME_R3;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return M_ICON_L1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return M_ICON_R1;
        case SDL_CONTROLLER_BUTTON_MISC1:         return M_NAME_MIC;
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
        return "";

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

static bool M_JoyBtn(const SDL_GameControllerButton button)
{
    if (m_Controller == nullptr) {
        return false;
    }
    return SDL_GameControllerGetButton(m_Controller, button);
}

static int16_t M_JoyAxis(const SDL_GameControllerAxis axis)
{
    if (m_Controller == nullptr) {
        return false;
    }
    const Sint16 value = SDL_GameControllerGetAxis(m_Controller, axis);
    if (value < -SDL_JOYSTICK_AXIS_MAX / 2) {
        return -1;
    }
    if (value > SDL_JOYSTICK_AXIS_MAX / 2) {
        return 1;
    }
    return 0;
}

static bool M_GetBindState(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    const CONTROLLER_MAP assigned = m_Layout[layout][role];
    if (assigned.type == BT_BUTTON) {
        return M_JoyBtn(assigned.bind.button);
    } else {
        return M_JoyAxis(assigned.bind.axis) == assigned.axis_dir;
    }
}

static int16_t M_GetUniqueBind(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    const CONTROLLER_MAP assigned = m_Layout[layout][role];
    if (assigned.type == BT_AXIS) {
        // Add SDL_CONTROLLER_BUTTON_MAX as an axis offset because button and
        // axis enum values overlap. Also offset depending on axis direction.
        if (assigned.axis_dir == -1) {
            return assigned.bind.axis + SDL_CONTROLLER_BUTTON_MAX;
        } else {
            return assigned.bind.axis + SDL_CONTROLLER_BUTTON_MAX + 10;
        }
    }
    return assigned.bind.button;
}

static int16_t M_GetAssignedButtonType(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Layout[layout][role].type;
}

static int16_t M_GetAssignedBind(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    const CONTROLLER_MAP assigned = m_Layout[layout][role];
    if (assigned.type == BT_BUTTON) {
        return assigned.bind.button;
    } else {
        return assigned.bind.axis;
    }
}

static int16_t M_GetAssignedAxisDir(
    const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Layout[layout][role].axis_dir;
}

static void M_AssignButton(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int16_t button)
{
    m_Layout[layout][role].type = BT_BUTTON;
    m_Layout[layout][role].bind.button = button;
    m_Layout[layout][role].axis_dir = 0;
    M_CheckConflicts(layout);
}

void M_AssignAxis(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const int16_t axis,
    const int16_t axis_dir)
{
    m_Layout[layout][role].type = BT_AXIS;
    m_Layout[layout][role].bind.axis = axis;
    m_Layout[layout][role].axis_dir = axis_dir;
    M_CheckConflicts(layout);
}

static bool M_CheckConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role1, const INPUT_ROLE role2)
{
    const int16_t bind1 = M_GetUniqueBind(layout, role1);
    const int16_t bind2 = M_GetUniqueBind(layout, role2);
    return bind1 == bind2;
}

static void M_AssignConflict(
    const INPUT_LAYOUT layout, const INPUT_ROLE role, const bool conflict)
{
    m_Conflicts[layout][role] = conflict;
}

static void M_CheckConflicts(const INPUT_LAYOUT layout)
{
    Input_ConflictHelper(layout, M_CheckConflict, M_AssignConflict);
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

static void M_Init(void)
{
    // first, reset the roles to null
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        m_Layout[INPUT_LAYOUT_DEFAULT][role] = (CONTROLLER_MAP) {
            BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID }, 0
        };
    }
    // then load actually defined default bindings
    for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
        const BUILTIN_CONTROLLER_LAYOUT *const builtin = &m_BuiltinLayout[i];
        m_Layout[INPUT_LAYOUT_DEFAULT][builtin->role] = builtin->map;
    }
    M_CheckConflicts(INPUT_LAYOUT_DEFAULT);

    for (int32_t layout = INPUT_LAYOUT_CUSTOM_1;
         layout < INPUT_LAYOUT_NUMBER_OF; layout++) {
        M_ResetLayout(layout);
    }

    int32_t result = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
    } else {
        M_Discover();
    }
}

static void M_Shutdown(void)
{
    if (m_Controller != nullptr) {
        SDL_GameControllerClose(m_Controller);
        m_Controller = nullptr;
    }
}

static void M_Discover(void)
{
    if (m_Controller != nullptr) {
        SDL_GameControllerClose(m_Controller);
        m_Controller = nullptr;
    }
    m_Controller = M_FindController();
}

static bool M_CustomUpdate(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    if (m_Controller == nullptr) {
        return false;
    }
    result->menu_back |= M_JoyBtn(SDL_CONTROLLER_BUTTON_Y);
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
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_ICON_L2;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_ICON_R2;
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_NAME_ZL;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_NAME_ZR;
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
        default:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? M_NAME_L_ANALOG_LEFT : M_NAME_L_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? M_NAME_L_ANALOG_UP : M_NAME_L_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? M_NAME_R_ANALOG_LEFT : M_NAME_R_ANALOG_RIGHT;
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? M_NAME_R_ANALOG_UP : M_NAME_R_ANALOG_DOWN;
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return M_NAME_L_TRIGGER;
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return M_NAME_R_TRIGGER;
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;

    }
    // clang-format on
    return "????";
}

static bool M_IsRoleConflicted(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    return m_Conflicts[layout][role];
}

static const char *M_GetName(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    const CONTROLLER_MAP check = m_Layout[layout][role];
    if (check.type == BT_BUTTON) {
        return M_GetButtonName(check.bind.button);
    } else {
        return M_GetAxisName(check.bind.axis, check.axis_dir);
    }
}

static void M_UnassignRole(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    M_AssignButton(layout, role, -1);
}

static bool M_AssignFromJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role,
    JSON_OBJECT *const bind_obj)
{
    int16_t button_type = M_GetAssignedButtonType(layout, role);
    button_type = JSON_ObjectGetInt(bind_obj, "button_type", button_type);

    int16_t bind = M_GetAssignedBind(layout, role);
    bind = JSON_ObjectGetInt(bind_obj, "bind", bind);

    int16_t axis_dir = M_GetAssignedAxisDir(layout, role);
    axis_dir = JSON_ObjectGetInt(bind_obj, "axis_dir", axis_dir);

    if (button_type == BT_BUTTON) {
        M_AssignButton(layout, role, bind);
    } else {
        M_AssignAxis(layout, role, bind, axis_dir);
    }
    return true;
}

static bool M_AssignToJSONObject(
    const INPUT_LAYOUT layout, const INPUT_ROLE role,
    JSON_OBJECT *const bind_obj)
{
    const int16_t default_button_type =
        M_GetAssignedButtonType(INPUT_LAYOUT_DEFAULT, role);
    const int16_t default_axis_dir =
        M_GetAssignedAxisDir(INPUT_LAYOUT_DEFAULT, role);
    const int16_t default_bind = M_GetAssignedBind(INPUT_LAYOUT_DEFAULT, role);

    const int16_t user_button_type = M_GetAssignedButtonType(layout, role);
    const int16_t user_axis_dir = M_GetAssignedAxisDir(layout, role);
    const int16_t user_bind = M_GetAssignedBind(layout, role);

    if (user_button_type == default_button_type
        && user_axis_dir == default_axis_dir && user_bind == default_bind) {
        return false;
    }

    JSON_ObjectAppendInt(bind_obj, "button_type", user_button_type);
    JSON_ObjectAppendInt(bind_obj, "bind", user_bind);
    JSON_ObjectAppendInt(bind_obj, "axis_dir", user_axis_dir);
    return true;
}

static void M_ResetLayout(const INPUT_LAYOUT layout)
{
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        const CONTROLLER_MAP default_btn = m_Layout[INPUT_LAYOUT_DEFAULT][role];
        m_Layout[layout][role] = default_btn;
    }
    M_CheckConflicts(layout);
}

static bool M_ReadAndAssign(const INPUT_LAYOUT layout, const INPUT_ROLE role)
{
    for (SDL_GameControllerButton button = 0;
         button < SDL_CONTROLLER_BUTTON_MAX; button++) {
        if (M_JoyBtn(button)) {
            M_AssignButton(layout, role, button);
            return true;
        }
    }
    for (SDL_GameControllerAxis axis = 0; axis < SDL_CONTROLLER_AXIS_MAX;
         axis++) {
        int16_t axis_dir = M_JoyAxis(axis);
        if (axis_dir != 0) {
            M_AssignAxis(layout, role, axis, axis_dir);
            return true;
        }
    }
    return false;
}

INPUT_BACKEND_IMPL g_Input_Controller = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .discover = M_Discover,
    .custom_update = M_CustomUpdate,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = M_IsRoleConflicted,
    .get_name = M_GetName,
    .unassign_role = M_UnassignRole,
    .assign_from_json_object = M_AssignFromJSONObject,
    .assign_to_json_object = M_AssignToJSONObject,
    .reset_layout = M_ResetLayout,
    .read_and_assign = M_ReadAndAssign,
};

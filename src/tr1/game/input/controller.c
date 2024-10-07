#include "game/input/controller.h"

#include "game/input/internal.h"

#include <libtrx/log.h>

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
    // clang-format off
    { INPUT_ROLE_UP,                { BT_BUTTON, { SDL_CONTROLLER_BUTTON_DPAD_UP },       0 } },
    { INPUT_ROLE_DOWN,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_DPAD_DOWN },     0 } },
    { INPUT_ROLE_LEFT,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_DPAD_LEFT },     0 } },
    { INPUT_ROLE_RIGHT,             { BT_BUTTON, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT },    0 } },
    { INPUT_ROLE_STEP_L,            { BT_AXIS,   { SDL_CONTROLLER_AXIS_TRIGGERLEFT },     1 } },
    { INPUT_ROLE_STEP_R,            { BT_AXIS,   { SDL_CONTROLLER_AXIS_TRIGGERRIGHT },    1 } },
    { INPUT_ROLE_SLOW,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER }, 0 } },
    { INPUT_ROLE_JUMP,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_X },             0 } },
    { INPUT_ROLE_ACTION,            { BT_BUTTON, { SDL_CONTROLLER_BUTTON_A },             0 } },
    { INPUT_ROLE_DRAW,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_Y },             0 } },
    { INPUT_ROLE_LOOK,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_LEFTSHOULDER },  0 } },
    { INPUT_ROLE_ROLL,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_B },             0 } },
    { INPUT_ROLE_OPTION,            { BT_BUTTON, { SDL_CONTROLLER_BUTTON_BACK },          0 } },
    { INPUT_ROLE_FLY_CHEAT,         { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_ITEM_CHEAT,        { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_LEVEL_SKIP_CHEAT,  { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_TURBO_CHEAT,       { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_PAUSE,             { BT_BUTTON, { SDL_CONTROLLER_BUTTON_START },         0 } },
    { INPUT_ROLE_CAMERA_FORWARD,    { BT_AXIS,   { SDL_CONTROLLER_AXIS_RIGHTY },          -1 } },
    { INPUT_ROLE_CAMERA_BACK,       { BT_AXIS,   { SDL_CONTROLLER_AXIS_RIGHTY },          1 } },
    { INPUT_ROLE_CAMERA_LEFT,       { BT_AXIS,   { SDL_CONTROLLER_AXIS_RIGHTX },          -1 } },
    { INPUT_ROLE_CAMERA_RIGHT,      { BT_AXIS,   { SDL_CONTROLLER_AXIS_RIGHTX },          1 } },
    { INPUT_ROLE_EQUIP_PISTOLS,     { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_EQUIP_SHOTGUN,     { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_EQUIP_MAGNUMS,     { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_EQUIP_UZIS,        { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_USE_SMALL_MEDI,    { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_USE_BIG_MEDI,      { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_SAVE,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_LOAD,              { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_FPS,               { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_BILINEAR,          { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_ENTER_CONSOLE,     { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_CHANGE_TARGET,     { BT_BUTTON, { SDL_CONTROLLER_BUTTON_LEFTSTICK },     0 } },
    { INPUT_ROLE_TOGGLE_UI,         { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { INPUT_ROLE_CAMERA_UP,         { BT_AXIS,   { SDL_CONTROLLER_AXIS_LEFTY },           -1 } },
    { INPUT_ROLE_CAMERA_DOWN,       { BT_AXIS,   { SDL_CONTROLLER_AXIS_LEFTY },           1 } },
    { INPUT_ROLE_TOGGLE_PHOTO_MODE, { BT_BUTTON, { SDL_CONTROLLER_BUTTON_INVALID },       0 } },
    { -1,                           { 0,         { 0 },                                   0 } },
    // clang-format on
};

static CONTROLLER_MAP m_Layout[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF];

static SDL_GameController *m_Controller = NULL;
static const char *m_ControllerName = NULL;
static SDL_GameControllerType m_ControllerType = SDL_CONTROLLER_TYPE_UNKNOWN;

static bool m_Conflicts[INPUT_LAYOUT_NUMBER_OF][INPUT_ROLE_NUMBER_OF] = {
    false
};

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
static bool M_Update(INPUT_STATE *result, INPUT_LAYOUT layout);
static bool M_IsPressed(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_IsRoleConflicted(INPUT_LAYOUT layout, INPUT_ROLE role);
static const char *M_GetName(INPUT_LAYOUT layout, INPUT_ROLE role);
static void M_UnassignRole(INPUT_LAYOUT layout, INPUT_ROLE role);
static bool M_AssignFromJSONObject(INPUT_LAYOUT layout, JSON_OBJECT *bind_obj);
static bool M_AssignToJSONObject(
    INPUT_LAYOUT layout, JSON_OBJECT *bind_obj, INPUT_ROLE role);
static void M_ResetLayout(INPUT_LAYOUT layout);
static bool M_ReadAndAssign(INPUT_LAYOUT layout, INPUT_ROLE role);

static const char *M_GetButtonName(const SDL_GameControllerButton button)
{
    // clang-format off
    switch (m_ControllerType) {
        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "";
                case SDL_CONTROLLER_BUTTON_A:             return "\206";
                case SDL_CONTROLLER_BUTTON_B:             return "\205";
                case SDL_CONTROLLER_BUTTON_X:             return "\207";
                case SDL_CONTROLLER_BUTTON_Y:             return "\204";
                case SDL_CONTROLLER_BUTTON_BACK:          return "CREATE";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L3";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R3";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "^";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "_";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203 ";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202 ";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "MIC";      /* PS5 microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "B";
                case SDL_CONTROLLER_BUTTON_B:             return "A";
                case SDL_CONTROLLER_BUTTON_X:             return "Y";
                case SDL_CONTROLLER_BUTTON_Y:             return "X";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUTTON";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUTTON";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203 ";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202 ";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "CAPTURE";  /* Nintendo Switch capture button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "A";
                case SDL_CONTROLLER_BUTTON_B:             return "B";
                case SDL_CONTROLLER_BUTTON_X:             return "X";
                case SDL_CONTROLLER_BUTTON_Y:             return "Y";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "XBOX"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUMPER";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUMPER";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203 ";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202 ";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "SHARE";    /* Xbox Series X share button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;

        default:
            switch (button) {
                case SDL_CONTROLLER_BUTTON_INVALID:       return "INVALID";
                case SDL_CONTROLLER_BUTTON_A:             return "A";
                case SDL_CONTROLLER_BUTTON_B:             return "B";
                case SDL_CONTROLLER_BUTTON_X:             return "X";
                case SDL_CONTROLLER_BUTTON_Y:             return "Y";
                case SDL_CONTROLLER_BUTTON_BACK:          return "BACK";
                case SDL_CONTROLLER_BUTTON_GUIDE:         return "HOME"; /* Home button*/
                case SDL_CONTROLLER_BUTTON_START:         return "START";
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:     return "L STICK";
                case SDL_CONTROLLER_BUTTON_RIGHTSTICK:    return "R STICK";
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return "L BUMPER";
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R BUMPER";
                case SDL_CONTROLLER_BUTTON_DPAD_UP:       return "\203 ";
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:     return "\202 ";
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:     return "\200 ";
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:    return "\201 ";
                case SDL_CONTROLLER_BUTTON_MISC1:         return "SHARE"; /* Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button */
                case SDL_CONTROLLER_BUTTON_PADDLE1:       return "PADDLE 1"; /* Xbox Elite paddle P1 (upper left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE2:       return "PADDLE 2"; /* Xbox Elite paddle P3 (upper right, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE3:       return "PADDLE 3"; /* Xbox Elite paddle P2 (lower left, facing the back) */
                case SDL_CONTROLLER_BUTTON_PADDLE4:       return "PADDLE 4"; /* Xbox Elite paddle P4 (lower right, facing the back) */
                case SDL_CONTROLLER_BUTTON_TOUCHPAD:      return "TOUCHPAD"; /* PS4/PS5 touchpad button */
                case SDL_CONTROLLER_BUTTON_MAX:           return "";
            }
            break;

    }
    // clang-format on
    return "????";
}

static bool M_JoyBtn(const SDL_GameControllerButton button)
{
    return SDL_GameControllerGetButton(m_Controller, button);
}

static int16_t M_JoyAxis(const SDL_GameControllerAxis axis)
{
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
    if (m_Controller != NULL) {
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
            if (result == NULL) {
                LOG_ERROR("Could not open controller: %s", SDL_GetError());
            }
            return result;
        }
    }

    return NULL;
}

static void M_Init(void)
{
    for (int32_t layout = INPUT_LAYOUT_DEFAULT; layout < INPUT_LAYOUT_NUMBER_OF;
         layout++) {
        for (int32_t i = 0; m_BuiltinLayout[i].role != (INPUT_ROLE)-1; i++) {
            const BUILTIN_CONTROLLER_LAYOUT *const builtin =
                &m_BuiltinLayout[i];
            m_Layout[layout][builtin->role] = builtin->map;
        }
        M_CheckConflicts(layout);
    }

    int32_t result = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);
    if (result < 0) {
        LOG_ERROR("Error while calling SDL_Init: 0x%lx", result);
    } else {
        m_Controller = M_FindController();
    }
}

static void M_Shutdown(void)
{
    if (m_Controller != NULL) {
        SDL_GameControllerClose(m_Controller);
        m_Controller = NULL;
    }
}

static bool M_Update(INPUT_STATE *const result, const INPUT_LAYOUT layout)
{
    if (m_Controller == NULL) {
        return false;
    }

    // clang-format off
    result->forward                |= M_GetBindState(layout, INPUT_ROLE_UP);
    result->back                   |= M_GetBindState(layout, INPUT_ROLE_DOWN);
    result->left                   |= M_GetBindState(layout, INPUT_ROLE_LEFT);
    result->right                  |= M_GetBindState(layout, INPUT_ROLE_RIGHT);
    result->step_left              |= M_GetBindState(layout, INPUT_ROLE_STEP_L);
    result->step_right             |= M_GetBindState(layout, INPUT_ROLE_STEP_R);
    result->slow                   |= M_GetBindState(layout, INPUT_ROLE_SLOW);
    result->jump                   |= M_GetBindState(layout, INPUT_ROLE_JUMP);
    result->action                 |= M_GetBindState(layout, INPUT_ROLE_ACTION);
    result->draw                   |= M_GetBindState(layout, INPUT_ROLE_DRAW);
    result->look                   |= M_GetBindState(layout, INPUT_ROLE_LOOK);
    result->roll                   |= M_GetBindState(layout, INPUT_ROLE_ROLL);
    result->option                 |= M_GetBindState(layout, INPUT_ROLE_OPTION);
    result->pause                  |= M_GetBindState(layout, INPUT_ROLE_PAUSE);
    result->toggle_photo_mode      |= M_GetBindState(layout, INPUT_ROLE_TOGGLE_PHOTO_MODE);
    result->camera_up              |= M_GetBindState(layout, INPUT_ROLE_CAMERA_UP);
    result->camera_down            |= M_GetBindState(layout, INPUT_ROLE_CAMERA_DOWN);
    result->camera_forward         |= M_GetBindState(layout, INPUT_ROLE_CAMERA_FORWARD);
    result->camera_back            |= M_GetBindState(layout, INPUT_ROLE_CAMERA_BACK);
    result->camera_left            |= M_GetBindState(layout, INPUT_ROLE_CAMERA_LEFT);
    result->camera_right           |= M_GetBindState(layout, INPUT_ROLE_CAMERA_RIGHT);
    result->item_cheat             |= M_GetBindState(layout, INPUT_ROLE_ITEM_CHEAT);
    result->fly_cheat              |= M_GetBindState(layout, INPUT_ROLE_FLY_CHEAT);
    result->level_skip_cheat       |= M_GetBindState(layout, INPUT_ROLE_LEVEL_SKIP_CHEAT);
    result->turbo_cheat            |= M_GetBindState(layout, INPUT_ROLE_TURBO_CHEAT);
    result->equip_pistols          |= M_GetBindState(layout, INPUT_ROLE_EQUIP_PISTOLS);
    result->equip_shotgun          |= M_GetBindState(layout, INPUT_ROLE_EQUIP_SHOTGUN);
    result->equip_magnums          |= M_GetBindState(layout, INPUT_ROLE_EQUIP_MAGNUMS);
    result->equip_uzis             |= M_GetBindState(layout, INPUT_ROLE_EQUIP_UZIS);
    result->use_small_medi         |= M_GetBindState(layout, INPUT_ROLE_USE_SMALL_MEDI);
    result->use_big_medi           |= M_GetBindState(layout, INPUT_ROLE_USE_BIG_MEDI);
    result->save                   |= M_GetBindState(layout, INPUT_ROLE_SAVE);
    result->load                   |= M_GetBindState(layout, INPUT_ROLE_LOAD);
    result->toggle_fps_counter     |= M_GetBindState(layout, INPUT_ROLE_FPS);
    result->toggle_bilinear_filter |= M_GetBindState(layout, INPUT_ROLE_BILINEAR);
    result->toggle_ui              |= M_GetBindState(layout, INPUT_ROLE_TOGGLE_UI);
    result->change_target          |= M_GetBindState(layout, INPUT_ROLE_CHANGE_TARGET);
    result->menu_confirm           |= M_JoyBtn(SDL_CONTROLLER_BUTTON_A);
    result->menu_back              |= M_JoyBtn(SDL_CONTROLLER_BUTTON_B);
    result->menu_back              |= M_JoyBtn(SDL_CONTROLLER_BUTTON_Y);
    // clang-format on
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
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "\300";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "{";
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "ZL";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "ZR";
                case SDL_CONTROLLER_AXIS_MAX:             return "";
            }
            break;

        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE:
        default:
            switch (axis) {
                case SDL_CONTROLLER_AXIS_INVALID:         return "";
                case SDL_CONTROLLER_AXIS_LEFTX:           return axis_dir == -1 ? "L ANALOG LEFT" : "L ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_LEFTY:           return axis_dir == -1 ? "L ANALOG UP" : "L ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_RIGHTX:          return axis_dir == -1 ? "R ANALOG LEFT" : "R ANALOG RIGHT";
                case SDL_CONTROLLER_AXIS_RIGHTY:          return axis_dir == -1 ? "R ANALOG UP" : "R ANALOG DOWN";
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:     return "L TRIGGER";
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:    return "R TRIGGER";
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
    const INPUT_LAYOUT layout, JSON_OBJECT *const bind_obj)
{
    const INPUT_ROLE role = JSON_ObjectGetInt(bind_obj, "role", -1);
    if (role == (INPUT_ROLE)-1) {
        return false;
    }

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
    const INPUT_LAYOUT layout, JSON_OBJECT *const bind_obj,
    const INPUT_ROLE role)
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

    JSON_ObjectAppendInt(bind_obj, "role", role);
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

INPUT_BACKEND_IMPL g_InputController = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .update = M_Update,
    .is_pressed = M_IsPressed,
    .is_role_conflicted = M_IsRoleConflicted,
    .get_name = M_GetName,
    .unassign_role = M_UnassignRole,
    .assign_from_json_object = M_AssignFromJSONObject,
    .assign_to_json_object = M_AssignToJSONObject,
    .reset_layout = M_ResetLayout,
    .read_and_assign = M_ReadAndAssign,
};

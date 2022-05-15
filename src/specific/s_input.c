#include "specific/s_input.h"

#include "config.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/shell.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "specific/s_shell.h"

#include <dinput.h>
#include <stdbool.h>

#define KEY_DOWN(a) ((m_DIKeys[(a)] & 0x80) != 0)

static INPUT_SCANCODE m_Layout[2][INPUT_ROLE_NUMBER_OF] = {
    // clang-format off
    // built-in controls
    {
        DIK_UP,          // INPUT_ROLE_UP
        DIK_DOWN,        // INPUT_ROLE_DOWN
        DIK_LEFT,        // INPUT_ROLE_LEFT
        DIK_RIGHT,       // INPUT_ROLE_RIGHT
        DIK_DELETE,      // INPUT_ROLE_STEP_L
        DIK_NEXT,        // INPUT_ROLE_STEP_R
        DIK_RSHIFT,      // INPUT_ROLE_SLOW
        DIK_RMENU,       // INPUT_ROLE_JUMP
        DIK_RCONTROL,    // INPUT_ROLE_ACTION
        DIK_SPACE,       // INPUT_ROLE_DRAW
        DIK_NUMPAD0,     // INPUT_ROLE_LOOK
        DIK_END,         // INPUT_ROLE_ROLL
        DIK_ESCAPE,      // INPUT_ROLE_OPTION
        DIK_O,           // INPUT_ROLE_FLY_CHEAT,
        DIK_I,           // INPUT_ROLE_ITEM_CHEAT,
        DIK_L,           // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        DIK_TAB,         // INPUT_ROLE_TURBO_CHEAT,
        DIK_P,           // INPUT_ROLE_PAUSE,
        DIK_W,           // INPUT_ROLE_CAMERA_UP
        DIK_S,           // INPUT_ROLE_CAMERA_DOWN
        DIK_A,           // INPUT_ROLE_CAMERA_LEFT
        DIK_D,           // INPUT_ROLE_CAMERA_RIGHT
        DIK_SLASH,       // INPUT_ROLE_CAMERA_RESET
    },

    // default user controls
    {
        DIK_NUMPAD8,     // INPUT_ROLE_UP
        DIK_NUMPAD2,     // INPUT_ROLE_DOWN
        DIK_NUMPAD4,     // INPUT_ROLE_LEFT
        DIK_NUMPAD6,     // INPUT_ROLE_RIGHT
        DIK_NUMPAD7,     // INPUT_ROLE_STEP_L
        DIK_NUMPAD9,     // INPUT_ROLE_STEP_R
        DIK_NUMPAD1,     // INPUT_ROLE_SLOW
        DIK_ADD,         // INPUT_ROLE_JUMP
        DIK_NUMPADENTER, // INPUT_ROLE_ACTION
        DIK_NUMPAD3,     // INPUT_ROLE_DRAW
        DIK_NUMPAD0,     // INPUT_ROLE_LOOK
        DIK_NUMPAD5,     // INPUT_ROLE_ROLL
        DIK_DECIMAL,     // INPUT_ROLE_OPTION
        DIK_O,           // INPUT_ROLE_FLY_CHEAT,
        DIK_I,           // INPUT_ROLE_ITEM_CHEAT,
        DIK_L,           // INPUT_ROLE_LEVEL_SKIP_CHEAT,
        DIK_TAB,         // INPUT_ROLE_TURBO_CHEAT,
        DIK_P,           // INPUT_ROLE_PAUSE,
        DIK_W,           // INPUT_ROLE_CAMERA_UP
        DIK_S,           // INPUT_ROLE_CAMERA_DOWN
        DIK_A,           // INPUT_ROLE_CAMERA_LEFT
        DIK_D,           // INPUT_ROLE_CAMERA_RIGHT
        DIK_SLASH,       // INPUT_ROLE_CAMERA_RESET
    }
    // clang-format on
};

static LPDIRECTINPUT8 m_DInput = NULL;
static LPDIRECTINPUTDEVICE8 m_IDID_SysKeyboard = NULL;
static LPDIRECTINPUTDEVICE8 m_IDID_Joystick = NULL;
static uint8_t m_DIKeys[256] = { 0 };

static bool S_Input_DInput_Create(void);
static void S_Input_DInput_Shutdown(void);
static bool S_Input_DInput_KeyboardCreate(void);
static void S_Input_DInput_KeyboardRelease(void);
static bool S_Input_DInput_KeyboardRead(void);
static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout);
static bool S_Input_Key(INPUT_ROLE role);

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode);
static HRESULT S_Input_DInput_JoystickCreate(void);
static void S_Input_DInput_JoystickRelease(void);
static BOOL CALLBACK
S_Input_EnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context);
static BOOL CALLBACK
S_Input_EnumCallback(LPCDIDEVICEINSTANCE instance, LPVOID context);

static const char *S_Input_GetScancodeName(INPUT_SCANCODE scancode)
{
    // clang-format off
    switch (scancode) {
        case DIK_ESCAPE:       return "ESC";
        case DIK_1:            return "1";
        case DIK_2:            return "2";
        case DIK_3:            return "3";
        case DIK_4:            return "4";
        case DIK_5:            return "5";
        case DIK_6:            return "6";
        case DIK_7:            return "7";
        case DIK_8:            return "8";
        case DIK_9:            return "9";
        case DIK_0:            return "0";
        case DIK_MINUS:        return "-";
        case DIK_EQUALS:       return "+";
        case DIK_BACK:         return "BKSP";
        case DIK_TAB:          return "TAB";
        case DIK_Q:            return "Q";
        case DIK_W:            return "W";
        case DIK_E:            return "E";
        case DIK_R:            return "R";
        case DIK_T:            return "T";
        case DIK_Y:            return "Y";
        case DIK_U:            return "U";
        case DIK_I:            return "I";
        case DIK_O:            return "O";
        case DIK_P:            return "P";
        case DIK_LBRACKET:     return "<";
        case DIK_RBRACKET:     return ">";
        case DIK_RETURN:       return "RET";
        case DIK_LCONTROL:     return "CTRL";
        case DIK_A:            return "A";
        case DIK_S:            return "S";
        case DIK_D:            return "D";
        case DIK_F:            return "F";
        case DIK_G:            return "G";
        case DIK_H:            return "H";
        case DIK_J:            return "J";
        case DIK_K:            return "K";
        case DIK_L:            return "L";
        case DIK_SEMICOLON:    return ";";
        case DIK_APOSTROPHE:   return "\'";
        case DIK_GRAVE:        return "`";
        case DIK_LSHIFT:       return "SHIFT";
        case DIK_BACKSLASH:    return "\\";
        case DIK_Z:            return "Z";
        case DIK_X:            return "X";
        case DIK_C:            return "C";
        case DIK_V:            return "V";
        case DIK_B:            return "B";
        case DIK_N:            return "N";
        case DIK_M:            return "M";
        case DIK_COMMA:        return ",";
        case DIK_PERIOD:       return ".";
        case DIK_SLASH:        return "/";
        case DIK_RSHIFT:       return "SHIFT";
        case DIK_MULTIPLY:     return "PADx";
        case DIK_LMENU:        return "ALT";
        case DIK_SPACE:        return "SPACE";
        case DIK_CAPITAL:      return "CAPS";
        case DIK_F1:           return "F1";
        case DIK_F2:           return "F2";
        case DIK_F3:           return "F3";
        case DIK_F4:           return "F4";
        case DIK_F5:           return "F5";
        case DIK_F6:           return "F6";
        case DIK_F7:           return "F7";
        case DIK_F8:           return "F8";
        case DIK_F9:           return "F9";
        case DIK_F10:          return "F10";
        case DIK_NUMLOCK:      return "NMLK";
        case DIK_SCROLL:       return "SCLK";
        case DIK_NUMPAD7:      return "PAD7";
        case DIK_NUMPAD8:      return "PAD8";
        case DIK_NUMPAD9:      return "PAD9";
        case DIK_SUBTRACT:     return "PAD-";
        case DIK_NUMPAD4:      return "PAD4";
        case DIK_NUMPAD5:      return "PAD5";
        case DIK_NUMPAD6:      return "PAD6";
        case DIK_ADD:          return "PAD+";
        case DIK_NUMPAD1:      return "PAD1";
        case DIK_NUMPAD2:      return "PAD2";
        case DIK_NUMPAD3:      return "PAD3";
        case DIK_NUMPAD0:      return "PAD0";
        case DIK_DECIMAL:      return "PAD.";
        case DIK_F11:          return "F11";
        case DIK_F12:          return "F12";
        case DIK_F13:          return "F13";
        case DIK_F14:          return "F14";
        case DIK_F15:          return "F15";
        case DIK_NUMPADEQUALS: return "PAD=";
        case DIK_AT:           return "@";
        case DIK_COLON:        return ":";
        case DIK_UNDERLINE:    return "_";
        case DIK_NUMPADENTER:  return "ENTER";
        case DIK_RCONTROL:     return "CTRL";
        case DIK_DIVIDE:       return "PAD/";
        case DIK_RMENU:        return "ALT";
        case DIK_HOME:         return "HOME";
        case DIK_UP:           return "UP";
        case DIK_PRIOR:        return "PGUP";
        case DIK_LEFT:         return "LEFT";
        case DIK_RIGHT:        return "RIGHT";
        case DIK_END:          return "END";
        case DIK_DOWN:         return "DOWN";
        case DIK_NEXT:         return "PGDN";
        case DIK_INSERT:       return "INS";
        case DIK_DELETE:       return "DEL";
    }
    // clang-format on
    return "????";
}

void S_Input_Init(void)
{
    if (!S_Input_DInput_Create()) {
        Shell_ExitSystem("Fatal DirectInput error!");
    }

    if (!S_Input_DInput_KeyboardCreate()) {
        Shell_ExitSystem("Fatal DirectInput error!");
    }

    if (g_Config.enable_xbox_one_controller) {
        S_Input_DInput_JoystickCreate();
    } else {
        m_IDID_Joystick = NULL;
    }
}

void S_Input_Shutdown(void)
{
    S_Input_DInput_KeyboardRelease();
    if (g_Config.enable_xbox_one_controller) {
        S_Input_DInput_JoystickRelease();
    }
    S_Input_DInput_Shutdown();
}

static bool S_Input_DInput_Create(void)
{
    HRESULT result = DirectInput8Create(
        g_TombModule, DIRECTINPUT_VERSION, &IID_IDirectInput8,
        (LPVOID *)&m_DInput, NULL);

    if (result) {
        LOG_ERROR("Error while calling DirectInput8Create: 0x%lx", result);
        return false;
    }

    return true;
}

static void S_Input_DInput_Shutdown(void)
{
    if (m_DInput) {
        IDirectInput_Release(m_DInput);
        m_DInput = NULL;
    }
}

bool S_Input_DInput_KeyboardCreate(void)
{
    HRESULT result = IDirectInput8_CreateDevice(
        m_DInput, &GUID_SysKeyboard, &m_IDID_SysKeyboard, NULL);
    if (result) {
        LOG_ERROR(
            "Error while calling IDirectInput8_CreateDevice: 0x%lx", result);
        return false;
    }

    result = IDirectInputDevice_SetCooperativeLevel(
        m_IDID_SysKeyboard, g_TombHWND, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    if (result) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_SetCooperativeLevel: 0x%lx",
            result);
        return false;
    }

    result =
        IDirectInputDevice_SetDataFormat(m_IDID_SysKeyboard, &c_dfDIKeyboard);
    if (result) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_SetDataFormat: 0x%lx",
            result);
        return false;
    }

    return true;
}

void S_Input_DInput_KeyboardRelease(void)
{
    if (m_IDID_SysKeyboard) {
        IDirectInputDevice_Unacquire(m_IDID_SysKeyboard);
        IDirectInputDevice_Release(m_IDID_SysKeyboard);
        m_IDID_SysKeyboard = NULL;
    }
}

static bool S_Input_DInput_KeyboardRead(void)
{
    if (!m_IDID_SysKeyboard) {
        return false;
    }

    while (IDirectInputDevice_GetDeviceState(
        m_IDID_SysKeyboard, sizeof(m_DIKeys), m_DIKeys)) {
        if (IDirectInputDevice_Acquire(m_IDID_SysKeyboard)) {
            memset(m_DIKeys, 0, sizeof(m_DIKeys));
            break;
        }
    }
    S_Shell_SpinMessageLoop();

    return true;
}

static bool S_Input_KbdKey(INPUT_ROLE role, INPUT_LAYOUT layout)
{
    INPUT_SCANCODE scancode = m_Layout[layout][role];
    if (KEY_DOWN(scancode)) {
        return true;
    }
    if (scancode == DIK_LCONTROL) {
        return KEY_DOWN(DIK_RCONTROL);
    }
    if (scancode == DIK_RCONTROL) {
        return KEY_DOWN(DIK_LCONTROL);
    }
    if (scancode == DIK_LSHIFT) {
        return KEY_DOWN(DIK_RSHIFT);
    }
    if (scancode == DIK_RSHIFT) {
        return KEY_DOWN(DIK_LSHIFT);
    }
    if (scancode == DIK_LMENU) {
        return KEY_DOWN(DIK_RMENU);
    }
    if (scancode == DIK_RMENU) {
        return KEY_DOWN(DIK_LMENU);
    }
    return false;
}

static bool S_Input_Key(INPUT_ROLE role)
{
    return S_Input_KbdKey(role, INPUT_LAYOUT_USER)
        || (!Input_IsKeyConflictedWithDefault(role)
            && S_Input_KbdKey(role, INPUT_LAYOUT_DEFAULT));
}

static HRESULT S_Input_DInput_JoystickCreate(void)
{
    HRESULT result;

    // Look for the first simple joystick we can find.
    if (FAILED(
            result = IDirectInput8_EnumDevices(
                m_DInput, DI8DEVCLASS_GAMECTRL, S_Input_EnumCallback, NULL,
                DIEDFL_ATTACHEDONLY))) {
        LOG_ERROR(
            "Error while calling IDirectInput8_EnumDevices: 0x%lx", result);
        return result;
    }

    // Make sure we got a joystick
    if (!m_IDID_Joystick) {
        LOG_ERROR("Joystick not found.\n");
        return E_FAIL;
    }
    LOG_INFO("Joystick found.\n");

    DIDEVCAPS capabilities;
    // request simple joystick format 2
    if (FAILED(
            result = IDirectInputDevice_SetDataFormat(
                m_IDID_Joystick, &c_dfDIJoystick2))) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_SetDataFormat: 0x%lx",
            result);
        S_Input_DInput_JoystickRelease();
        return result;
    }

    // don't request exclusive access
    if (FAILED(
            result = IDirectInputDevice_SetCooperativeLevel(
                m_IDID_Joystick, NULL,
                DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_SetCooperativeLevel: 0x%lx",
            result);
        S_Input_DInput_JoystickRelease();
        return result;
    }

    // get axis count, we should know what it is but best to ask
    capabilities.dwSize = sizeof(DIDEVCAPS);
    if (FAILED(
            result = IDirectInputDevice_GetCapabilities(
                m_IDID_Joystick, &capabilities))) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_GetCapabilities: 0x%lx",
            result);
        S_Input_DInput_JoystickRelease();
        return result;
    }

    // set the range we expect each axis to report back in
    if (FAILED(
            result = IDirectInputDevice_EnumObjects(
                m_IDID_Joystick, S_Input_EnumAxesCallback, NULL, DIDFT_AXIS))) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice_EnumObjects: 0x%lx",
            result);
        S_Input_DInput_JoystickRelease();
        return result;
    }
    return result;
}

static void S_Input_DInput_JoystickRelease(void)
{
    if (m_IDID_Joystick) {
        IDirectInputDevice_Unacquire(m_IDID_Joystick);
        IDirectInputDevice_Release(m_IDID_Joystick);
        m_IDID_Joystick = NULL;
    }
}

static BOOL CALLBACK
S_Input_EnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE instance, LPVOID context)
{
    HRESULT result;
    DIPROPRANGE propRange;

    propRange.diph.dwSize = sizeof(DIPROPRANGE);
    propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    propRange.diph.dwHow = DIPH_BYID;
    propRange.diph.dwObj = instance->dwType;
    propRange.lMin = -1024;
    propRange.lMax = 1024;

    // Set the range for the axis
    if (FAILED(
            result = IDirectInputDevice8_SetProperty(
                m_IDID_Joystick, DIPROP_RANGE, &propRange.diph))) {
        LOG_ERROR(
            "Error while calling IDirectInputDevice8_SetProperty: 0x%lx",
            result);
        return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK
S_Input_EnumCallback(LPCDIDEVICEINSTANCE instance, LPVOID context)
{
    HRESULT result;

    // Obtain an interface to the enumerated joystick.
    result = IDirectInput8_CreateDevice(
        m_DInput, &instance->guidInstance, &m_IDID_Joystick, NULL);

    if (FAILED(result)) {
        LOG_ERROR(
            "Error while calling IDirectInput8_CreateDevice: 0x%lx", result);
        return DIENUM_CONTINUE;
    }
    // we got one, it will do.
    return DIENUM_STOP;
}

static HRESULT DInputJoystickPoll(DIJOYSTATE2 *joystate)
{
    HRESULT result;

    if (!m_IDID_Joystick) {
        return S_OK;
    }

    // Poll the device to read the current state
    result = IDirectInputDevice8_Poll(m_IDID_Joystick);
    if (FAILED(result)) {
        // focus was lost, try to reaquire the device
        result = IDirectInputDevice8_Acquire(m_IDID_Joystick);
        while (result == DIERR_INPUTLOST) {
            result = IDirectInputDevice8_Acquire(m_IDID_Joystick);
        }

        // A fatal error? Return failure.
        if ((result == DIERR_INVALIDPARAM)
            || (result == DIERR_NOTINITIALIZED)) {
            LOG_ERROR(
                "Error while calling IDirectInputDevice8_Acquire: 0x%lx",
                result);
            return E_FAIL;
        }

        // If another application has control of this device, return
        // successfully.
        if (result == DIERR_OTHERAPPHASPRIO) {
            return S_OK;
        }
    }

    // Get the input's device state
    if (FAILED(
            result = IDirectInputDevice8_GetDeviceState(
                m_IDID_Joystick, sizeof(DIJOYSTATE2), joystate))) {
        return result; // The device should have been acquired during the Poll()
    }

    return S_OK;
}

INPUT_STATE S_Input_GetCurrentState(void)
{
    S_Input_DInput_KeyboardRead();

    INPUT_STATE linput = { 0 };

    linput.forward = S_Input_Key(INPUT_ROLE_UP);
    linput.back = S_Input_Key(INPUT_ROLE_DOWN);
    linput.left = S_Input_Key(INPUT_ROLE_LEFT);
    linput.right = S_Input_Key(INPUT_ROLE_RIGHT);
    linput.step_left = S_Input_Key(INPUT_ROLE_STEP_L);
    linput.step_right = S_Input_Key(INPUT_ROLE_STEP_R);
    linput.slow = S_Input_Key(INPUT_ROLE_SLOW);
    linput.jump = S_Input_Key(INPUT_ROLE_JUMP);
    linput.action = S_Input_Key(INPUT_ROLE_ACTION);
    linput.draw = S_Input_Key(INPUT_ROLE_DRAW);
    linput.look = S_Input_Key(INPUT_ROLE_LOOK);
    linput.roll = S_Input_Key(INPUT_ROLE_ROLL);
    linput.option = S_Input_Key(INPUT_ROLE_OPTION);
    linput.pause = S_Input_Key(INPUT_ROLE_PAUSE);
    linput.camera_up = S_Input_Key(INPUT_ROLE_CAMERA_UP);
    linput.camera_down = S_Input_Key(INPUT_ROLE_CAMERA_DOWN);
    linput.camera_left = S_Input_Key(INPUT_ROLE_CAMERA_LEFT);
    linput.camera_right = S_Input_Key(INPUT_ROLE_CAMERA_RIGHT);
    linput.camera_reset = S_Input_Key(INPUT_ROLE_CAMERA_RESET);

    linput.item_cheat = S_Input_Key(INPUT_ROLE_ITEM_CHEAT);
    linput.fly_cheat = S_Input_Key(INPUT_ROLE_FLY_CHEAT);
    linput.level_skip_cheat = S_Input_Key(INPUT_ROLE_LEVEL_SKIP_CHEAT);
    linput.turbo_cheat = S_Input_Key(INPUT_ROLE_TURBO_CHEAT);
    linput.health_cheat = KEY_DOWN(DIK_F11);

    linput.equip_pistols = KEY_DOWN(DIK_1);
    linput.equip_shotgun = KEY_DOWN(DIK_2);
    linput.equip_magnums = KEY_DOWN(DIK_3);
    linput.equip_uzis = KEY_DOWN(DIK_4);
    linput.use_small_medi = KEY_DOWN(DIK_8);
    linput.use_big_medi = KEY_DOWN(DIK_9);

    linput.select = KEY_DOWN(DIK_RETURN);
    linput.deselect = S_Input_Key(INPUT_ROLE_OPTION);

    linput.save = KEY_DOWN(DIK_F5);
    linput.load = KEY_DOWN(DIK_F6);

    linput.toggle_fps_counter = KEY_DOWN(DIK_F2);
    linput.toggle_bilinear_filter = KEY_DOWN(DIK_F3);
    linput.toggle_perspective_filter = KEY_DOWN(DIK_F4);

    if (m_IDID_Joystick) {
        DIJOYSTATE2 state;
        DInputJoystickPoll(&state);

        // check Y
        if (state.lY > 512) {
            linput.back = 1;
        } else if (state.lY < -512) {
            linput.forward = 1;
        }
        // check X
        if (state.lX > 512) {
            linput.right = 1;
        } else if (state.lX < -512) {
            linput.left = 1;
        }
        // check Z
        if (state.lZ > 512) {
            linput.step_left = 1;
        } else if (state.lZ < -512) {
            linput.step_right = 1;
        }

        // check 2nd stick X
        if (state.lRx > 512) {
            linput.camera_right = 1;
        } else if (state.lRx < -512) {
            linput.camera_left = 1;
        }
        // check 2nd stick Y
        if (state.lRy > 512) {
            linput.camera_down = 1;
        } else if (state.lRy < -512) {
            linput.camera_up = 1;
        }

        // check buttons
        if (state.rgbButtons[0]) { // A
            linput.jump = 1;
            linput.select = 1;
        }
        if (state.rgbButtons[1]) { // B
            linput.roll = 1;
            linput.deselect = 1;
        }
        if (state.rgbButtons[2]) { // X
            linput.action = 1;
            linput.select = 1;
        }
        if (state.rgbButtons[3]) { // Y
            linput.look = 1;
            linput.deselect = 1;
        }
        if (state.rgbButtons[4]) { // LB
            linput.slow = 1;
        }
        if (state.rgbButtons[5]) { // RB
            linput.draw = 1;
        }
        if (state.rgbButtons[6]) { // back
            linput.option = 1;
        }
        if (state.rgbButtons[7]) { // start
            linput.pause = 1;
        }
        if (state.rgbButtons[9]) { // 2nd axis click
            linput.camera_reset = 1;
        }
        // check dpad
        if (state.rgdwPOV[0] == 0) { // up
            linput.draw = 1;
        }
    }

    return linput;
}

INPUT_SCANCODE S_Input_GetAssignedScancode(int16_t layout_num, INPUT_ROLE role)
{
    return m_Layout[layout_num][role];
}

void S_Input_AssignScancode(
    int16_t layout_num, INPUT_ROLE role, INPUT_SCANCODE scancode)
{
    m_Layout[layout_num][role] = scancode;
}

bool S_Input_ReadAndAssignKey(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    for (INPUT_SCANCODE scancode = 0; scancode < 256; scancode++) {
        if (KEY_DOWN(scancode)) {
            m_Layout[layout_num][role] = scancode;
            return true;
        }
    }
    return false;
}

const char *S_Input_GetKeyName(INPUT_LAYOUT layout_num, INPUT_ROLE role)
{
    return S_Input_GetScancodeName(m_Layout[layout_num][role]);
}

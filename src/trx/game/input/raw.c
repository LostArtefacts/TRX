#include <trx/game/input/raw.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/input/common.h>
#include <trx/game/input/raw_state.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_keyboard.h>
#include <string.h>

static bool m_KeyDown[SDL_NUM_SCANCODES] = {};
static bool m_KeyPressed[SDL_NUM_SCANCODES] = {};
static bool m_ButtonDown[SDL_CONTROLLER_BUTTON_MAX] = {};
static bool m_ButtonPressed[SDL_CONTROLLER_BUTTON_MAX] = {};
static int16_t m_Axis[SDL_CONTROLLER_AXIS_MAX] = {};
static char *m_KeyName = nullptr;

static INPUT_RAW_TRACKER m_Keys = {
    .down = m_KeyDown,
    .pressed = m_KeyPressed,
    .count = SDL_NUM_SCANCODES,
};

static INPUT_RAW_TRACKER m_Buttons = {
    .down = m_ButtonDown,
    .pressed = m_ButtonPressed,
    .count = SDL_CONTROLLER_BUTTON_MAX,
};

// Whether a script holds the devices, so that the game stops acting on them.
static bool m_ScriptHold = false;

// Scancode of the key that prints the named character, or that carries the
// named label. Taking the character first is what makes "5" the key labelled 5
// rather than the key that prints 5 on a US keyboard.
static SDL_Scancode M_ScancodeFromName(const char *const key)
{
    if (key == nullptr || key[0] == '\0') {
        return SDL_SCANCODE_UNKNOWN;
    }

    AUTO_FREE char *lower = String_ToLower(key);
    const SDL_Keycode keycode = SDL_GetKeyFromName(lower);
    if (keycode != SDLK_UNKNOWN) {
        const SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode);
        if (scancode != SDL_SCANCODE_UNKNOWN) {
            return scancode;
        }
    }
    return SDL_GetScancodeFromName(lower);
}

static void M_ProcessKeyEvent(const SDL_Event *const event)
{
    const SDL_Scancode scancode = event->key.keysym.scancode;
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return;
    }
    // A held key repeats, and a repeat is neither a press nor a release.
    if (event->type == SDL_KEYDOWN && event->key.repeat) {
        return;
    }
    InputRawState_Set(&m_Keys, scancode, event->type == SDL_KEYDOWN);
}

static void M_ProcessButtonEvent(const SDL_Event *const event)
{
    InputRawState_Set(
        &m_Buttons, event->cbutton.button,
        event->type == SDL_CONTROLLERBUTTONDOWN);
}

// Takes up whether the game holds the devices. Every path that records or
// reads state calls this first, so that the keys, the buttons and the axes
// answer the same question at the moment it changes rather than a frame later.
static void M_SyncReserved(void)
{
    const bool reserved = Input_IsInListenMode();
    m_Keys.reserved = reserved;
    m_Buttons.reserved = reserved;
}

void InputRaw_BeginFrame(void)
{
    M_SyncReserved();
    InputRawState_BeginFrame(&m_Keys);
    InputRawState_BeginFrame(&m_Buttons);
}

void InputRaw_ProcessEvent(const SDL_Event *const event)
{
    M_SyncReserved();
    switch (event->type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        M_ProcessKeyEvent(event);
        break;

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        M_ProcessButtonEvent(event);
        break;

    case SDL_CONTROLLERAXISMOTION:
        if (event->caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
            m_Axis[event->caxis.axis] = event->caxis.value;
        }
        break;

    default:
        break;
    }
}

bool InputRaw_IsReserved(void)
{
    M_SyncReserved();
    return m_Keys.reserved;
}

void InputRaw_SetScriptHold(const bool held)
{
    if (m_ScriptHold == held) {
        return;
    }
    m_ScriptHold = held;
    if (!held) {
        // The key that gave the devices back is still down, and would read as
        // a fresh press the moment the game looks again.
        Input_Settle();
    }
}

bool InputRaw_IsHeldByScript(void)
{
    return m_ScriptHold;
}

void InputRaw_ForEachDown(
    const INPUT_RAW_DEVICES devices,
    void (*const fn)(INPUT_RAW_INPUT input, void *user_data),
    void *const user_data)
{
    if ((devices & INPUT_RAW_DEVICE_KEYBOARD) != 0) {
        for (int32_t i = 0; i < SDL_NUM_SCANCODES; i++) {
            if (!InputRawState_IsHeldRaw(&m_Keys, i)) {
                continue;
            }
            const char *const name = SDL_GetKeyName(SDL_GetKeyFromScancode(i));
            if (name == nullptr || name[0] == '\0') {
                continue;
            }
            AUTO_FREE char *lower = String_ToLower(name);
            fn((INPUT_RAW_INPUT) { .name = lower }, user_data);
        }
    }

    if ((devices & INPUT_RAW_DEVICE_CONTROLLER) != 0) {
        for (int32_t i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            if (!InputRawState_IsHeldRaw(&m_Buttons, i)) {
                continue;
            }
            const char *const name = SDL_GameControllerGetStringForButton(i);
            if (name == nullptr || name[0] == '\0') {
                continue;
            }
            fn((INPUT_RAW_INPUT) { .is_button = true, .name = name },
               user_data);
        }
    }
}

void InputRaw_ClearDevices(const INPUT_RAW_DEVICES devices)
{
    if ((devices & INPUT_RAW_DEVICE_KEYBOARD) != 0) {
        InputRawState_Clear(&m_Keys);
    }
    if ((devices & INPUT_RAW_DEVICE_CONTROLLER) != 0) {
        InputRawState_Clear(&m_Buttons);
        memset(m_Axis, 0, sizeof m_Axis);
    }
}

bool InputRaw_IsKeyHeld(const char *const key)
{
    M_SyncReserved();
    const SDL_Scancode scancode = M_ScancodeFromName(key);
    return scancode != SDL_SCANCODE_UNKNOWN
        && InputRawState_IsHeld(&m_Keys, scancode);
}

bool InputRaw_IsKeyPressed(const char *const key)
{
    M_SyncReserved();
    const SDL_Scancode scancode = M_ScancodeFromName(key);
    return scancode != SDL_SCANCODE_UNKNOWN
        && InputRawState_IsPressed(&m_Keys, scancode);
}

bool InputRaw_IsKeyKnown(const char *const key)
{
    return M_ScancodeFromName(key) != SDL_SCANCODE_UNKNOWN;
}

const char *InputRaw_EventKeyName(const SDL_Event *const event)
{
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) {
        return nullptr;
    }

    const char *const name = SDL_GetKeyName(event->key.keysym.sym);
    if (name == nullptr || name[0] == '\0') {
        return nullptr;
    }

    // Keep the name in a buffer of its own. A script that a key event reaches
    // can read more keys before the next script sees the same event, so a
    // shared buffer would hand the later script another key's name.
    Memory_FreePointer(&m_KeyName);
    m_KeyName = String_ToLower(name);
    return m_KeyName;
}

bool InputRaw_IsButtonHeld(const char *const button)
{
    M_SyncReserved();
    const SDL_GameControllerButton index =
        SDL_GameControllerGetButtonFromString(button);
    return InputRawState_IsHeld(&m_Buttons, index);
}

bool InputRaw_IsButtonPressed(const char *const button)
{
    M_SyncReserved();
    const SDL_GameControllerButton index =
        SDL_GameControllerGetButtonFromString(button);
    return InputRawState_IsPressed(&m_Buttons, index);
}

bool InputRaw_IsButtonKnown(const char *const button)
{
    return SDL_GameControllerGetButtonFromString(button)
        != SDL_CONTROLLER_BUTTON_INVALID;
}

float InputRaw_GetAxis(const char *const axis)
{
    if (InputRaw_IsReserved()) {
        return 0.0f;
    }
    const SDL_GameControllerAxis index =
        SDL_GameControllerGetAxisFromString(axis);
    if (index == SDL_CONTROLLER_AXIS_INVALID) {
        return 0.0f;
    }
    return InputRawState_ScaleAxis(m_Axis[index]);
}

bool InputRaw_IsAxisKnown(const char *const axis)
{
    return SDL_GameControllerGetAxisFromString(axis)
        != SDL_CONTROLLER_AXIS_INVALID;
}

const char *InputRaw_EventButtonName(const SDL_Event *const event)
{
    if (event->type != SDL_CONTROLLERBUTTONDOWN
        && event->type != SDL_CONTROLLERBUTTONUP) {
        return nullptr;
    }
    return SDL_GameControllerGetStringForButton(event->cbutton.button);
}

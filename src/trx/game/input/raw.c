#include <trx/game/input/raw.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>
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

void InputRaw_BeginFrame(void)
{
    InputRawState_BeginFrame(&m_Keys);
    InputRawState_BeginFrame(&m_Buttons);
}

void InputRaw_ProcessEvent(const SDL_Event *const event)
{
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

    case SDL_CONTROLLERDEVICEREMOVED:
        // A pad that goes away reports nothing more, so a button it held and
        // a stick it left off centre would read that way for good.
        InputRawState_Clear(&m_Buttons);
        memset(m_Axis, 0, sizeof m_Axis);
        break;

    default:
        break;
    }
}

bool InputRaw_IsKeyHeld(const char *const key)
{
    const SDL_Scancode scancode = M_ScancodeFromName(key);
    return scancode != SDL_SCANCODE_UNKNOWN
        && InputRawState_IsHeld(&m_Keys, scancode);
}

bool InputRaw_IsKeyPressed(const char *const key)
{
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
    const SDL_GameControllerButton index =
        SDL_GameControllerGetButtonFromString(button);
    return InputRawState_IsHeld(&m_Buttons, index);
}

bool InputRaw_IsButtonPressed(const char *const button)
{
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

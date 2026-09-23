#pragma once

// Read keyboard and controller state as hardware input. Use this interface
// when a script needs a physical key, button, or axis instead of a game action.
// Name keys by the character that the current layout prints. Keep named keys
// in lower case. Use SDL names for buttons and axes. Read state from events so
// replay input follows live input.
//
// While a rebind is reading the devices, the game holds them rather than the
// player: every read below reports nothing, and the presses that arrive go
// unrecorded. See InputRaw_IsReserved. A script holding the devices is a
// separate thing that leaves the reads answering; see InputRaw_IsHeldByScript.

#include <stdint.h>

// Keep SDL_Event opaque because only the event stream reads event fields.
typedef union SDL_Event SDL_Event;

// Names the devices an operation covers.
typedef enum {
    // clang-format off
    INPUT_RAW_DEVICE_KEYBOARD   = 1 << 0,
    INPUT_RAW_DEVICE_CONTROLLER = 1 << 1,
    INPUT_RAW_DEVICE_ALL        =
        INPUT_RAW_DEVICE_KEYBOARD | INPUT_RAW_DEVICE_CONTROLLER,
    // clang-format on
} INPUT_RAW_DEVICES;

// Names a key or a button, for walking what is down.
typedef struct {
    // True for a controller button, false for a key.
    bool is_button;
    const char *name;
} INPUT_RAW_INPUT;

// Drops the presses of the frame before, and takes up whether the game holds
// the devices.
void InputRaw_BeginFrame(void);

// Tracks a key, a button or an axis from an event.
void InputRaw_ProcessEvent(const SDL_Event *event);

// Checks whether the game holds the devices rather than the player, which it
// does while a rebind is reading them.
bool InputRaw_IsReserved(void);

// Controls whether a script holds the devices. While one does, the game stops
// acting on them, but the reads above keep answering and the events keep
// firing, so that the script can drive an interface of its own. The game
// taking them still wins, and reports nothing to anybody.
void InputRaw_SetScriptHold(bool held);

// Checks whether a script holds the devices.
bool InputRaw_IsHeldByScript(void);

// Reports each key and button of the named devices that is physically down,
// whoever holds them. This is what balances the presses and releases a script
// was told about when the game takes the devices and gives them back.
void InputRaw_ForEachDown(
    INPUT_RAW_DEVICES devices,
    void (*fn)(INPUT_RAW_INPUT input, void *user_data), void *user_data);

// Lets go of what the named devices held, for devices that stop reporting: the
// window losing focus, or a pad being unplugged. Clearing the controller brings
// its axes to rest.
void InputRaw_ClearDevices(INPUT_RAW_DEVICES devices);

// Checks whether the named key is down. An unknown name reports false.
bool InputRaw_IsKeyHeld(const char *key);

// Checks whether the named key went down in this frame. An unknown name
// reports false.
bool InputRaw_IsKeyPressed(const char *key);

// Checks whether a key name is one the player's layout has a key for.
bool InputRaw_IsKeyKnown(const char *key);

// Names the key a keyboard event carries, or returns nullptr for an event that
// carries none. The name holds until the next call.
const char *InputRaw_EventKeyName(const SDL_Event *event);

// Checks whether the named controller button is down. An unknown name reports
// false.
bool InputRaw_IsButtonHeld(const char *button);

// Checks whether the named controller button went down in this frame. An
// unknown name reports false.
bool InputRaw_IsButtonPressed(const char *button);

// Checks whether a controller button name is one SDL knows.
bool InputRaw_IsButtonKnown(const char *button);

// Position of the named axis, from -1 to 1, or 0 for a trigger at rest, for an
// unknown name and for a controller that is not attached. A trigger runs from
// 0 to 1.
float InputRaw_GetAxis(const char *axis);

// Checks whether a controller axis name is one SDL knows.
bool InputRaw_IsAxisKnown(const char *axis);

// Names the button a controller event carries, or returns nullptr for an event
// that carries none.
const char *InputRaw_EventButtonName(const SDL_Event *event);

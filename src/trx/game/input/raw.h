#pragma once

// Read keyboard and controller state as hardware input. Use this interface
// when a script needs a physical key, button, or axis instead of a game action.
// Name keys by the character that the current layout prints. Keep named keys
// in lower case. Use SDL names for buttons and axes. Read state from events so
// replay input follows live input.

#include <stdint.h>

// Keep SDL_Event opaque because only the event stream reads event fields.
typedef union SDL_Event SDL_Event;

// Clear press and release state from the previous frame.
void InputRaw_BeginFrame(void);

// Tracks a key, a button or an axis from an event, and forgets what a
// controller held once it goes away.
void InputRaw_ProcessEvent(const SDL_Event *event);

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

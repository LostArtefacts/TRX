#pragma once

// Reading the keyboard as hardware rather than as game actions. A role answers
// what the player wants to do, and is what game code and most scripts want;
// this answers which key is down, for a script that needs the key itself, such
// as one reading a passcode off a keypad.
//
// Keys are named by the character the player's layout prints, so the key
// labelled 5 is "5" on every layout, and named keys keep the spelling SDL gives
// them in lower case, such as "escape" and "left shift".
//
// State is held per scancode and fed from the event stream rather than read
// from the window system, so a recording drives it exactly as a player does.

#include <stdint.h>

// Keep SDL_Event opaque because only the event stream reads event fields.
typedef union SDL_Event SDL_Event;

// Clear press and release state from the previous frame.
void InputRaw_BeginFrame(void);

// Tracks a key going down or coming up.
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

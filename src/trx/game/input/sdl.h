#pragma once

// The parts of input that speak in SDL's own types: feeding it events, and
// converting between scancodes and the key descriptions bindings are stored as.
// They sit apart from the rest so that reading the input state, or naming a
// bound key, needs no window system.

#include <trx/game/input/enum.h>

#include <SDL2/SDL_events.h>

// Processes a SDL event to update global input state before polling.
// @param event     Event to process.
void Input_ProcessEvent(const SDL_Event *event);

// Serialize a scancode and modifier mask into a human-readable key
// description, e.g. "ctrl+shift+up". The returned string must not be held onto.
const char *Input_KeyDescFromSDL(SDL_Scancode scancode, SDL_Keymod mod);

// Parse a human-readable key description into scancode and modifier mask.
// e.g. "ctrl+shift+up" → scancode SDL_SCANCODE_UP, mod KMOD_CTRL|KMOD_SHIFT.
// Returns true if parsing succeeded, false otherwise.
bool Input_ParseKeyDesc(
    const char *desc, SDL_Scancode *scancode, SDL_Keymod *mod);

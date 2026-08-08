#pragma once

// Isolated platform-sensitive initialization code
#include <trx/core/vector.h>

#include <SDL2/SDL_video.h>

void Shell_SetupHiDPI(void);
void Shell_SetupLibAV(void);

// The languages the player's system is set to, most wanted first, as a vector
// of char * holding codes like "pl" or "pt-br". Empty where the system does
// not say. The caller owns the vector and each string in it.
VECTOR *Shell_GetPreferredLanguages(void);

void Shell_EnableThemeSupport(SDL_Window *window);

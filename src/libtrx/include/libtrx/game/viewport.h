#pragma once

#include <stdint.h>

extern int32_t Viewport_GetWidth(void);
extern int32_t Viewport_GetHeight(void);
extern int32_t Viewport_GetMaxX(void);
extern int32_t Viewport_GetMaxY(void);

// Return the current FOV as overriden by the game mechanics, such as special
// cameras or cutscenes. If the FOV is not overriden, returns -1.
extern int16_t Viewport_GetSystemFOV(void);

// Returns preferred player FOV.
extern int16_t Viewport_GetUserFOV(void);

// Returns the current effective FOV – eg system FOV if it's defined, otherwise
// the player choice.
int16_t Viewport_GetEffectiveFOV(void);

// Sets the system FOV. Set to -1 to fallback to player FOV.
extern void Viewport_AlterFOV(int16_t view_angle);

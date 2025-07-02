#pragma once

#include <stdint.h>

typedef enum {
    VIEWPORT_WINDOW,
    VIEWPORT_TARGET,
    VIEWPORT_GAME,
    VIEWPORT_NUMBER_OF,
} VIEWPORT_SPACE;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} VIEWPORT_RECT;

// TODO: hide this variable eventually
extern VIEWPORT_RECT *g_Viewport_Rects;

int32_t Viewport_GetWidth(VIEWPORT_SPACE space);
int32_t Viewport_GetHeight(VIEWPORT_SPACE space);
int32_t Viewport_GetMinX(VIEWPORT_SPACE space);
int32_t Viewport_GetMinY(VIEWPORT_SPACE space);
int32_t Viewport_GetMaxX(VIEWPORT_SPACE space);
int32_t Viewport_GetMaxY(VIEWPORT_SPACE space);
int32_t Viewport_GetCenterX(VIEWPORT_SPACE space);
int32_t Viewport_GetCenterY(VIEWPORT_SPACE space);
VIEWPORT_RECT Viewport_GetRect(VIEWPORT_SPACE space);

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

// TODO: decide what to do with this function
void Viewport_Reset(void);
void Viewport_ResetCommon(void);
void Viewport_Debug(void);

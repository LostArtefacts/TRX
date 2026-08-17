#pragma once

#include <trx/config/enum.h>

#include <stdint.h>

typedef enum {
    FOV_MODE_VERTICAL,
    FOV_MODE_HORIZONTAL,
    FOV_MODE_PC,
    FOV_MODE_PS1,
    // Like FOV_MODE_PS1, except the horizontal field of view stops narrowing
    // once the viewport is taller than 16:10, and the vertical one widens
    // instead.
    FOV_MODE_PS1_FIT,
} FOV_MODE;

// The rectangles the frame passes through, from the outside in. VIEWPORT_UI
// and VIEWPORT_SCENE differ by the upscaling factor, VIEWPORT_SCENE and
// VIEWPORT_GAME by the supersampling factor.
typedef enum {
    // The whole window, including any letterboxing.
    VIEWPORT_WINDOW,
    // The part of the window the frame is presented to.
    VIEWPORT_TARGET,
    // The pixel grid the player sees the scene as, before magnification.
    VIEWPORT_SCENE,
    // The pixel grid the scene is rasterized on.
    VIEWPORT_GAME,
    // The pixel grid the UI is rasterized on.
    VIEWPORT_UI,
    VIEWPORT_NUMBER_OF,
} VIEWPORT_SPACE;

typedef struct {
    int32_t x;
    int32_t y;
    union {
        int32_t width, w;
    };
    union {
        int32_t height, h;
    };
} VIEWPORT_RECT;

void Viewport_Init(int32_t x, int32_t y, int32_t width, int32_t height);

int32_t Viewport_GetWidth(VIEWPORT_SPACE space);
int32_t Viewport_GetHeight(VIEWPORT_SPACE space);
int32_t Viewport_GetMinX(VIEWPORT_SPACE space);
int32_t Viewport_GetMinY(VIEWPORT_SPACE space);
int32_t Viewport_GetMaxX(VIEWPORT_SPACE space);
int32_t Viewport_GetMaxY(VIEWPORT_SPACE space);
int32_t Viewport_GetCenterX(VIEWPORT_SPACE space);
int32_t Viewport_GetCenterY(VIEWPORT_SPACE space);
VIEWPORT_RECT Viewport_GetRect(VIEWPORT_SPACE space);

// Returns how many rasterized pixels the scene draws per pixel the player
// sees, along one axis. Anything sized in the pixels the driver rasterizes
// with, such as the line width, has to be multiplied by it to keep the width
// it had before supersampling.
int32_t Viewport_GetSupersamplingFactor(void);

// Return the current FOV as overriden by the game mechanics, such as special
// cameras or cutscenes. If the FOV is not overriden, returns -1.
int16_t Viewport_GetSystemFOV(void);

// Returns preferred player FOV.
int16_t Viewport_GetUserFOV(void);

// Returns the current effective FOV – eg system FOV if it's defined, otherwise
// the player choice.
int16_t Viewport_GetEffectiveFOV(void);

// Suspends the supersampling factor, for pictures that are magnified from a
// fixed source and so gain nothing from being rasterized above the resolution
// they are shown at. VIEWPORT_GAME then matches VIEWPORT_SCENE.
void Viewport_SetSupersamplingEnabled(bool enabled);

// Returns the current FOV formula.
FOV_MODE Viewport_GetFOVMode(void);

// Sets the system FOV. Set to -1 to fallback to player FOV.
void Viewport_AlterFOV(int16_t view_angle, FOV_MODE fov_mode);

// TODO: decide what to do with this function
void Viewport_Reset(void);

void Viewport_Debug(void);

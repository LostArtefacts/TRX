#pragma once

#include <trx/game/input/common.h>

#include <SDL2/SDL_events.h>

void TouchOverlay_Init(void);
void TouchOverlay_Shutdown(void);

void TouchOverlay_SetVisible(bool visible);
bool TouchOverlay_IsVisible(void);

void TouchOverlay_Draw(void);

// Returns true if the event was consumed by the touch overlay.
bool TouchOverlay_ProcessEvent(const SDL_Event *event);

// Returns true if any finger is currently touching the screen.
bool TouchOverlay_HasAnyFingerDown(void);

// Metadata exposed to the input backend for default binding generation.
int32_t TouchOverlay_GetPositionCount(void);
INPUT_ROLE TouchOverlay_GetPositionDefaultRole(int32_t position);

// Selection mode for touch remap listen phase.
// In selection mode, finger-down events record which position was tapped
// instead of triggering input actions.
void TouchOverlay_EnterSelectionMode(void);
void TouchOverlay_ExitSelectionMode(void);
int32_t TouchOverlay_GetSelectedPosition(void); // returns -1 if none selected

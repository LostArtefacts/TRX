#pragma once

#include <trx/config/enum.h>
#include <trx/core/colors.h>
#include <trx/game/ui/tween.h>

#include <stdint.h>

// Fade layers: full-screen layers that the engine draws above its interface
// and below the script painters above it, such as the console. A fade layer
// holds a colour or a snapshot of the frame, and its opacity moves on a tween.
// Fade layers stack in the order they were pushed.
//
// A backdrop layer is drawn under the interface instead. It holds one of the
// backdrop styles that the engine draws behind its screens, and its opacity
// is how far the backdrop has come in.
//
// A fade layer is named by an id. An id outlives its layer: once the layer
// closes, the id names nothing, and every call with it does nothing.

#define UI_FADE_NONE 0

// Pushes a layer of one colour.
int32_t UI_Fade_PushFill(RGBA_8888 color, float opacity);

// Pushes a layer that holds the frame on screen now. Returns UI_FADE_NONE
// when the frame cannot be captured.
int32_t UI_Fade_PushSnapshot(float opacity);

// Pushes a backdrop layer of a style, over the picture at `path` or a snapshot
// of the game that it captures as it is pushed.
int32_t UI_Fade_PushBackdrop(
    BACKGROUND_TYPE style, const char *path, float opacity);

// Makes the engine wait to start its next phase while the layer is open.
void UI_Fade_HoldScreen(int32_t id);

// Returns whether an open layer makes the engine wait to start its next phase.
bool UI_Fade_IsHoldingScreen(void);

bool UI_Fade_IsOpen(int32_t id);
float UI_Fade_GetOpacity(int32_t id);

// Moves the opacity to a target. A layer told to close does so once the move
// ends.
void UI_Fade_To(
    int32_t id, float target, int32_t ticks, UI_EASE ease, bool close);

void UI_Fade_Close(int32_t id);

// Schedules the open layers for drawing, in push order.
void UI_Fade_Draw(void);

// Reports each move that has ended since the last call, once, through
// `on_done`, then closes the layers that were told to close. Runs once a tick.
void UI_Fade_Tick(void (*on_done)(int32_t id));

#pragma once

#include <stdint.h>

typedef enum {
    UI_SCALER_TARGET_GENERIC,
    UI_SCALER_TARGET_BAR,
    UI_SCALER_TARGET_TEXT,
} UI_SCALER_TARGET;

double UI_Scaler_GetScale(const UI_SCALER_TARGET target);
float UI_Scaler_Calc(float unit, UI_SCALER_TARGET target);
float UI_Scaler_CalcInverse(float unit, UI_SCALER_TARGET target);

// The size everything text-shaped is drawn at: the player's text scale, times
// the fit factor currently pushed. Widgets size themselves from this
// rather than from the config, so a subtree can be scaled as a whole.
float UI_Scaler_GetTextScale(void);

// Returns the player's text scale, without any fit factor pushed over it. The
// screen edges are drawn at this scale, so the area left to a dialog is
// measured in it.
float UI_Scaler_GetBaseTextScale(void);

// The fit factor on its own, without the player's text scale. Widgets that
// carry their own scale - a label - fold this into it, because they are
// measured after the tree is built and the factor is no longer pushed by then.
float UI_Scaler_GetContentScale(void);

// Scale the widgets created until the matching pop by an extra factor. A dialog
// too wide for the canvas pushes the factor that brings it back within it;
// nesting multiplies.
void UI_Scaler_PushTextScale(float factor);
void UI_Scaler_PopTextScale(void);

#pragma once

#define UI_LOADING_BAR_HEIGHT 14.0f

typedef struct {
    // Width in canvas units.
    float w;

    // Height in canvas units. Zero takes the default height at the bars scale.
    float h;

    // How much of the bar is filled, from 0 to 1.
    float progress;
} UI_LOADING_BAR_SETTINGS;

// Draws the loading bar: a progress bar that keeps a plain white frame
// whichever bars appearance is chosen.
void UI_LoadingBar(UI_LOADING_BAR_SETTINGS settings);

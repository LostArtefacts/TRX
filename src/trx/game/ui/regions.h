#pragma once

// Names the nine screen regions available to widgets.
//
//     top-left      top-center      top-right
//     left          center          right
//     bottom-left   bottom-center   bottom-right
//
// Each region stacks the slots reserved in it away from the screen edge, the
// first slot nearest the top. The center region is reserved for dialogs.

#include <stdint.h>

typedef enum {
    UI_REGION_TOP_LEFT,
    UI_REGION_TOP_CENTER,
    UI_REGION_TOP_RIGHT,
    UI_REGION_LEFT,
    UI_REGION_CENTER,
    UI_REGION_RIGHT,
    UI_REGION_BOTTOM_LEFT,
    UI_REGION_BOTTOM_CENTER,
    UI_REGION_BOTTOM_RIGHT,
    UI_REGION_NUMBER_OF,
} UI_REGION;

// Forgets the slots of the last scene. Called as a scene begins.
void UI_Region_Clear(void);

// Lays the slots out and updates the middle box. Called once per scene, after
// every slot is reserved, so that the middle box describes the current scene.
void UI_Region_Layout(void);

// Returns the middle box from the last call to UI_Region_Layout.
void UI_Region_GetCenterBox(float *x, float *y, float *w, float *h);

// Returns the middle box as a box of the given width, centered in it, sees
// it: a region at the top or the bottom takes room from it only if the box
// would reach that region. A negative width is as wide as the middle box.
void UI_Region_GetCenterBoxFor(
    float width, float *x, float *y, float *w, float *h);

// Reserves space in a region and returns its slot for the current scene, or
// -1 where the scene has no room left for one.
int32_t UI_Region_Reserve(UI_REGION region, float w, float h);

// Returns a reservation box from the last layout, or false for a stale slot.
bool UI_Region_GetSlotBox(int32_t slot, float *x, float *y, float *w, float *h);

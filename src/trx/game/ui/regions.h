#pragma once

// Names the nine screen regions available to widgets.
//
//     top-left      top-center      top-right
//     left          center          right
//     bottom-left   bottom-center   bottom-right
//
// The eight edge regions stack their children away from the edge. The center
// region is reserved for dialogs.

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

// Opens a region and stacks children there until UI_EndRegion.
// Opening the same region twice in one scene reuses its stack. Regions may not
// be nested.
void UI_BeginRegion(UI_REGION region);
void UI_EndRegion(void);

// Returns the middle box from the last laid-out scene.
void UI_Region_GetCenterBox(float *x, float *y, float *w, float *h);

// Reserves space in a region and returns its slot for the current scene.
int32_t UI_Region_Reserve(UI_REGION region, float w, float h);

// Returns a reservation box from the last layout, or false for a stale slot.
bool UI_Region_GetSlotBox(int32_t slot, float *x, float *y, float *w, float *h);

// Returns whether a region has no children in the current scene.
bool UI_Region_IsEmpty(UI_REGION region);

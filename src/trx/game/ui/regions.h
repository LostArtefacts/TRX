#pragma once

// Names the nine screen regions available to widgets.
//
//     top-left      top-center      top-right
//     left          center          right
//     bottom-left   bottom-center   bottom-right
//
// The eight edge regions stack their children away from the edge. The center
// region is reserved for dialogs.

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

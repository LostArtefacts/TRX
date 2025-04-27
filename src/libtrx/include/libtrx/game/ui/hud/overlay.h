#pragma once

// Ingame user interface display widget.

#include "../common.h"

typedef enum {
    UI_OVERLAY_ARROW_TL,
    UI_OVERLAY_ARROW_TR,
    UI_OVERLAY_ARROW_BL,
    UI_OVERLAY_ARROW_BR,
} UI_OVERLAY_ARROW;

typedef struct UI_OVERLAY_STATE UI_OVERLAY_STATE;

// state functions
extern UI_OVERLAY_STATE *UI_Overlay_Init(void);
extern void UI_Overlay_Free(UI_OVERLAY_STATE *s);
extern void UI_Overlay_Control(UI_OVERLAY_STATE *s);

// draw functions
extern void UI_Overlay(UI_OVERLAY_STATE *s);
extern void UI_BeginOverlayRegion(float x, float y);
extern void UI_EndOverlayRegion(void);

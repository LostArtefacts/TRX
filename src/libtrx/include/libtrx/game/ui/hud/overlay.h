#pragma once

// Ingame user interface display widget.

#include "../common.h"

typedef enum {
    UI_OVERLAY_ARROW_TL, // top-left screen corner
    UI_OVERLAY_ARROW_TR, // top-right screen corner
    UI_OVERLAY_ARROW_BL, // bottom-left screen corner
    UI_OVERLAY_ARROW_BR, // bottom-right screen corner
    UI_OVERLAY_ARROW_BCL, // low text left side
    UI_OVERLAY_ARROW_BCR, // low text right side
} UI_OVERLAY_ARROW;

typedef struct UI_OVERLAY_STATE UI_OVERLAY_STATE;

// state functions
UI_OVERLAY_STATE *UI_Overlay_Init(void);
void UI_Overlay_Free(UI_OVERLAY_STATE *s);
void UI_Overlay_Control(UI_OVERLAY_STATE *s);

// draw functions
void UI_Overlay(UI_OVERLAY_STATE *s);
void UI_BeginOverlayRegion(float x, float y);
void UI_EndOverlayRegion(void);

void UI_Overlay_ForceHealthBar(UI_OVERLAY_STATE *s, bool show);
void UI_Overlay_ShowArrow(
    UI_OVERLAY_STATE *s, UI_OVERLAY_ARROW arrow, bool show);
void UI_Overlay_ShowVersion(UI_OVERLAY_STATE *s, bool show);
void UI_Overlay_SetTopText(UI_OVERLAY_STATE *s, const char *text, bool flash);
void UI_Overlay_SetBottomText(
    UI_OVERLAY_STATE *s, const char *text, bool flash);

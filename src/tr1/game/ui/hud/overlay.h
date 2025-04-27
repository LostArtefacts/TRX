#pragma once

#include <libtrx/game/ui/hud/overlay.h>

void UI_Overlay_ForceHealthBar(UI_OVERLAY_STATE *s, bool show);
void UI_Overlay_ShowArrows(
    UI_OVERLAY_STATE *s, UI_OVERLAY_ARROW arrow, bool show);
void UI_Overlay_SetBottomText(
    UI_OVERLAY_STATE *s, const char *text, bool flash);

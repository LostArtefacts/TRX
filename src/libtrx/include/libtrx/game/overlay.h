#pragma once

#include "./ui/hud/overlay.h"

extern void Overlay_Control(void);
extern void Overlay_Draw(void);

extern void Overlay_MakeAmmoString(char *string);
extern void Overlay_HideGameInfo(void);
extern void Overlay_DrawModeInfo(void);

extern void Overlay_ShowArrows(UI_OVERLAY_ARROW arrow, bool show);
extern void Overlay_SetBottomText(const char *text, bool flash);

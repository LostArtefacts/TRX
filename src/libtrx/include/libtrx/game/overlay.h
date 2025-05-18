#pragma once

#include "./ui/hud/overlay.h"

void Overlay_Init(void);
void Overlay_Shutdown(void);

extern void Overlay_Reset(void);
void Overlay_Control(void);
void Overlay_Draw(void);

extern void Overlay_HideGameInfo(void);

void Overlay_ForceHealthBar(bool show);
void Overlay_SetHealthBarTimer(int16_t health_bar_timer);
void Overlay_ShowArrow(UI_OVERLAY_ARROW arrow, bool show);
void Overlay_ShowVersion(bool show);
void Overlay_SetTopText(const char *text, bool flash);
void Overlay_SetBottomText(const char *text, bool flash);

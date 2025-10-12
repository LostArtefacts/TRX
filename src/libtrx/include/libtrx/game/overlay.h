#pragma once

#include "./objects/types.h"
#include "./ui/hud/overlay.h"

void Overlay_Init(void);
void Overlay_Shutdown(void);

void Overlay_Reset(void);
void Overlay_Control(void);
void Overlay_Animate(int32_t num_frames);
void Overlay_Draw(void);

void Overlay_DrawGameInfo(void);
void Overlay_AddDisplayPickup(OBJECT_ID obj_id);

void Overlay_ForceHealthBar(bool show);
void Overlay_SetHealthBarTimer(int16_t health_bar_timer);
void Overlay_ShowArrow(UI_OVERLAY_ARROW arrow, bool show);
void Overlay_ShowVersion(bool show);

void Overlay_SetTopText(const char *text, bool flash);
void Overlay_SetBottomText(const char *text, bool flash);

// Like Overlay_SetTopText(), but takes a stable indirect pointer.
void Overlay_SetTopTextPtr(const char *const *ptr, bool flash);
// Like Overlay_SetBottomText(), but takes a stable indirect pointer.
void Overlay_SetBottomTextPtr(const char *const *ptr, bool flash);

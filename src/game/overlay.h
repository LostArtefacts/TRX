#pragma once

#include "game/screen.h"
#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

struct BAR_INFO;

void Overlay_Init(void);

void Overlay_BarSetHealthTimer(int16_t health_bar_timer);
void Overlay_BarHealthTimerTick(void);
void Overlay_BarDraw(BAR_INFO *bar_info, RENDER_SCALE_REF scale_func);
void Overlay_BarDrawHealth(void);

void Overlay_HideGameInfo(void);
void Overlay_DrawGameInfo(void);
void Overlay_DrawFPSInfo(void);

void Overlay_AddPickup(int16_t object_num);

void Overlay_MakeAmmoString(char *string);
